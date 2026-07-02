#include "../src/ccad_core/refdes_tracker.hpp"
#include <cassert>
#include <iostream>

using namespace ccad;

static void test_basic_insertion() {
    std::cout << "  test_basic_insertion... ";
    RefdesTracker tracker;

    assert( tracker.insert("R1") == true );
    assert( tracker.contains("R1") == true );
    assert( tracker.insert("R1") == false ); // Duplicate

    assert( tracker.insert("C2") == true );
    assert( tracker.insert("IC10") == true );
    assert( tracker.insert("IC11") == true );

    assert( tracker.size() == 4 );

    tracker.clear();
    assert( tracker.size() == 0 );
    assert( tracker.contains("R1") == false );
    std::cout << "PASS\n";
}

static void test_prefix_next_available() {
    std::cout << "  test_prefix_next_available... ";
    RefdesTracker tracker;

    assert( tracker.get_next_refdes("R") == 1 );
    assert( tracker.contains("R1") == true );

    assert( tracker.get_next_refdes("R") == 2 );
    assert( tracker.contains("R2") == true );

    tracker.insert("R4");
    
    // R3 is available
    assert( tracker.get_next_refdes("R") == 3 );
    
    // R5 is available
    assert( tracker.get_next_refdes("R") == 5 );

    // Min value testing
    assert( tracker.get_next_refdes("R", 10) == 10 );
    assert( tracker.get_next_refdes("R", 10) == 11 );
    std::cout << "PASS\n";
}

static void test_serialization() {
    std::cout << "  test_serialization... ";
    RefdesTracker tracker;

    tracker.insert("R1");
    tracker.insert("R2");
    tracker.insert("R3");
    tracker.insert("R5");
    tracker.insert("C1");
    tracker.insert("IC"); // Prefix only

    std::string serialized = tracker.serialize();
    
    RefdesTracker tracker2;
    assert( tracker2.deserialize(serialized) == true );

    assert( tracker2.size() == 6 );
    assert( tracker2.contains("R1") == true );
    assert( tracker2.contains("R2") == true );
    assert( tracker2.contains("R3") == true );
    assert( tracker2.contains("R5") == true );
    assert( tracker2.contains("C1") == true );
    assert( tracker2.contains("IC") == true );
    assert( tracker2.contains("R4") == false );
    std::cout << "PASS\n";
}

int main() {
    std::cout << "Refdes Tracker Tests\n";
    std::cout << "====================\n";

    test_basic_insertion();
    test_prefix_next_available();
    test_serialization();

    std::cout << "\nAll 3 refdes tests passed!\n";
    return 0;
}
