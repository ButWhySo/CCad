#include "agent_runner.hpp"
#include "ccad_core/json.hpp"
#include <iostream>
#include <fstream>
#include <regex>
#include <stdexcept>
// Include json serialization helper (assumed similar to other core files)

namespace ccad {

namespace {

size_t matching_json_delimiter(const std::string& text, size_t start);

std::string unescape_json_string(const std::string& value) {
    std::string out;
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '\\' || i + 1 >= value.size()) {
            out.push_back(value[i]);
            continue;
        }
        const char escaped = value[++i];
        switch (escaped) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            default: out.push_back(escaped); break;
        }
    }
    return out;
}

std::string json_string_field(const std::string& object, const std::string& key,
                              const std::string& fallback = {}) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"((?:\\\\.|[^\\\"\\\\])*)\\\"");
    std::smatch match;
    if (!std::regex_search(object, match, pattern)) return fallback;
    return unescape_json_string(match[1].str());
}

std::string json_raw_field(const std::string& object, const std::string& key,
                           const std::string& fallback = {}) {
    const size_t key_pos = object.find("\"" + key + "\"");
    if (key_pos == std::string::npos) return fallback;
    const size_t colon = object.find(':', key_pos);
    if (colon == std::string::npos) return fallback;
    const size_t value_start = object.find_first_not_of(" \t\r\n", colon + 1);
    if (value_start == std::string::npos) return fallback;
    if (object[value_start] == '"') return "\"" + ccad::escapeJson(json_string_field(object, key)) + "\"";
    if (object[value_start] == '{' || object[value_start] == '[') {
        const size_t value_end = matching_json_delimiter(object, value_start);
        return object.substr(value_start, value_end - value_start + 1);
    }
    const size_t value_end = object.find_first_of(",}", value_start);
    return object.substr(value_start, value_end == std::string::npos ? std::string::npos : value_end - value_start);
}

int json_int_field(const std::string& object, const std::string& key, int fallback = 0) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+)");
    std::smatch match;
    if (!std::regex_search(object, match, pattern)) return fallback;
    return std::stoi(match[1].str());
}

size_t matching_json_delimiter(const std::string& text, size_t start) {
    const char opening = text[start];
    const char closing = opening == '{' ? '}' : ']';
    int depth = 0;
    bool quoted = false;
    bool escaped = false;
    for (size_t i = start; i < text.size(); ++i) {
        const char ch = text[i];
        if (quoted) {
            if (escaped) escaped = false;
            else if (ch == '\\') escaped = true;
            else if (ch == '"') quoted = false;
            continue;
        }
        if (ch == '"') quoted = true;
        else if (ch == opening) ++depth;
        else if (ch == closing && --depth == 0) return i;
    }
    throw std::runtime_error("invalid agent queue JSON: unterminated value");
}

std::vector<std::string> object_values_in_array(const std::string& array) {
    std::vector<std::string> values;
    for (size_t i = 1; i + 1 < array.size();) {
        while (i + 1 < array.size() && (array[i] == ',' || std::isspace(static_cast<unsigned char>(array[i])))) ++i;
        if (i + 1 >= array.size() || array[i] == ']') break;
        if (array[i] != '{') throw std::runtime_error("invalid agent queue JSON: expected object");
        const size_t end = matching_json_delimiter(array, i);
        values.push_back(array.substr(i, end - i + 1));
        i = end + 1;
    }
    return values;
}

AgentTask load_task(const std::string& object) {
    AgentTask task;
    task.id = json_string_field(object, "id");
    task.description = json_string_field(object, "description");
    task.tool_name = json_string_field(object, "tool_name");
    task.tool_args_json = json_raw_field(object, "tool_args", "{}");
    task.order = json_int_field(object, "order");
    if (task.id.empty() || task.tool_name.empty()) {
        throw std::runtime_error("invalid agent queue JSON: task identity missing");
    }
    return task;
}

AgentGoal load_goal(const std::string& object) {
    AgentGoal goal;
    goal.id = json_string_field(object, "id");
    goal.description = json_string_field(object, "description");
    goal.context_json = json_raw_field(object, "context", "{}");
    const size_t tasks_key = object.find("\"tasks\"");
    if (tasks_key != std::string::npos) {
        const size_t array_start = object.find('[', tasks_key);
        if (array_start == std::string::npos) throw std::runtime_error("invalid agent queue JSON: tasks array missing");
        const size_t array_end = matching_json_delimiter(object, array_start);
        for (const auto& task_object : object_values_in_array(object.substr(array_start, array_end - array_start + 1))) {
            goal.tasks.push_back(load_task(task_object));
        }
    }
    goal.total_count = static_cast<int>(goal.tasks.size());
    if (goal.id.empty() || goal.tasks.empty()) throw std::runtime_error("invalid agent queue JSON: goal incomplete");
    return goal;
}

}  // namespace

AgentRunner::AgentRunner() {}

AgentRunner::~AgentRunner() {
    stop();
}

void AgentRunner::start() {
    if (!running_) {
        running_ = true;
        worker_thread_ = std::thread(&AgentRunner::execution_loop, this);
    }
}

void AgentRunner::stop() {
    if (running_) {
        running_ = false;
        cv_.notify_all();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }
}

void AgentRunner::enqueue_goal(const AgentGoal& goal) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        pending_goals_.push(goal);
    }
    cv_.notify_one();
}

void AgentRunner::set_progress_callback(ProgressCallback cb) {
    on_progress_ = cb;
}

void AgentRunner::set_task_executor(TaskExecutor ex) {
    task_executor_ = ex;
}

void AgentRunner::save_queue(const std::string& filepath) const {
    std::ofstream out(filepath);
    if (!out) return;
    out << "{\"pending_goals\":[";
    
    // Create a copy of the queue to iterate over without popping from the original
    std::queue<AgentGoal> temp_queue;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        temp_queue = pending_goals_;
    }
    
    bool first = true;
    while (!temp_queue.empty()) {
        if (!first) out << ",";
        out << temp_queue.front().to_json();
        temp_queue.pop();
        first = false;
    }
    
    out << "]}";
    out.close();
}

void AgentRunner::load_queue(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in) throw std::runtime_error("cannot open agent queue: " + filepath);
    const std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const size_t goals_key = json.find("\"pending_goals\"");
    if (goals_key == std::string::npos) throw std::runtime_error("invalid agent queue JSON: pending_goals missing");
    const size_t array_start = json.find('[', goals_key);
    if (array_start == std::string::npos) throw std::runtime_error("invalid agent queue JSON: goals array missing");
    const size_t array_end = matching_json_delimiter(json, array_start);
    const size_t root_end = json.find_first_not_of(" \t\r\n", array_end + 1);
    if (root_end == std::string::npos || json[root_end] != '}' ||
        json.find_first_not_of(" \t\r\n", root_end + 1) != std::string::npos) {
        throw std::runtime_error("invalid agent queue JSON: trailing data");
    }
    auto goals = object_values_in_array(json.substr(array_start, array_end - array_start + 1));
    std::lock_guard<std::mutex> lock(queue_mutex_);
    for (const auto& goal_object : goals) pending_goals_.push(load_goal(goal_object));
    cv_.notify_all();
}

void AgentRunner::execution_loop() {
    while (running_) {
        AgentGoal current_goal;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] { return !pending_goals_.empty() || !running_; });
            
            if (!running_) break;
            
            current_goal = pending_goals_.front();
            pending_goals_.pop();
        }
        
        // Execute tasks inside the goal
        for (auto& task : current_goal.tasks) {
            if (!running_) break;
            
            if (task_executor_) {
                task = task_executor_(current_goal, task.id);
                if (task.status == TaskStatus::Failed) {
                    current_goal.failed_count++;
                } else if (task.status == TaskStatus::Completed) {
                    current_goal.completed_count++;
                }
            } else {
                // A queued task has no executable meaning without the real
                // orchestrator-owned executor.  Never manufacture a success,
                // sleep, retry, or approval state here: callers must install
                // the executor that owns policy, tools, transactions, and
                // evidence before starting a runner.
                task.status = TaskStatus::Failed;
                task.error_message = "task_executor_unavailable";
                current_goal.failed_count++;
            }
            
            if (on_progress_) {
                on_progress_(current_goal);
            }
        }
        if (current_goal.failed_count > 0 || current_goal.tasks.empty()) {
            current_goal.status = GoalStatus::Failed;
        } else {
            current_goal.status = GoalStatus::Completed;
        }
        if (on_progress_) {
            on_progress_(current_goal);
        }
    }
}

} // namespace ccad
