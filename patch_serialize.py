import re

def patch_file(file_path):
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Need to replace the broken writePadstack with the correct one.
    
    # Let's just fix the multiline strings by replacing literal newlines inside quotes.
    # Actually, it's easier to just use `multi_replace_file_content` via Python.
    
    fixed_write_padstack_impl = r"""
static void writePadstack(std::ostream& out, const int indent, const Padstack& padstack) {
  const std::string pad(indent, ' ');
  out << "{\n";
  
  out << pad << "  \"layer_set\": [";
  for (std::size_t i = 0; i < padstack.layer_set.size(); ++i) {
    out << "\"\\" << escapeJson(padstack.layer_set[i]) << "\\\"\"" << (i + 1 == padstack.layer_set.size() ? "" : ", ");
  }
  out << "],\n";

  out << pad << "  \"copper_props\": {\n";
  std::size_t c = 0;
  for (auto it = padstack.copper_props.begin(); it != padstack.copper_props.end(); ++it, ++c) {
    out << pad << "    \"" << escapeJson(it->first) << "\": {\n";
    out << pad << "      \"shape\": {\n";
    const auto& sp = it->second.shape;
    std::string shape_str = "circle";
    if (sp.shape == PadShape::Rectangle) shape_str = "rect";
    else if (sp.shape == PadShape::Oval) shape_str = "oval";
    else if (sp.shape == PadShape::Trapezoid) shape_str = "trapezoid";
    else if (sp.shape == PadShape::RoundRect) shape_str = "roundrect";
    else if (sp.shape == PadShape::ChamferedRect) shape_str = "chamfered_rect";
    else if (sp.shape == PadShape::Custom) shape_str = "custom";
    out << pad << "        \"shape\": \"" << shape_str << "\",\n";
    out << pad << "        \"size\": ";
    writeSize(out, indent + 8, sp.size);
    out << ",\n";
    out << pad << "        \"roundrect_rratio\": " << sp.roundrect_rratio << ",\n";
    out << pad << "        \"chamfer_ratio\": " << sp.chamfer_ratio << "\n";
    out << pad << "      }\n";
    out << pad << "    }" << (c + 1 == padstack.copper_props.size() ? "" : ",") << "\n";
  }
  out << pad << "  },\n";

  out << pad << "  \"drill\": {\n";
  out << pad << "    \"size\": ";
  writeSize(out, indent + 4, padstack.drill.size);
  out << "\n";
  out << pad << "  }\n";
  out << pad << "}";
}
"""
    # Replace the existing writePadstack
    start_idx = content.find("static void writePadstack(")
    end_idx = content.find("std::string dumpProjectJson(", start_idx)
    
    if start_idx != -1 and end_idx != -1:
        content = content[:start_idx] + fixed_write_padstack_impl + content[end_idx:]

    # Fix the missing readRawJsonElement error
    # It says "readRawJsonElement was not declared in this scope; did you mean readRawJsonObject?"
    content = content.replace("readRawJsonElement();", "readRawJsonObject();")

    with open(file_path, "w", encoding="utf-8") as f:
        f.write(content)

if __name__ == "__main__":
    patch_file(r"F:\CCad\src\ccad_core\serialize.cpp")
