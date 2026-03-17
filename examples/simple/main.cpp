#include <string>
#include <vector>

#include "../../include/serialization.hpp"

struct Position {
    float x {};
    float y {};
    float z {};

    template <ser::IsArchiveT ArchiveT>
    void serialize(ArchiveT& ar) {
        ar.property("x", x);
        ar.property("y", y);
        ar.property("z", z);
    }
};

struct Rotation {
    float yawn {};
    float pitch {};
    float roll {};

    template <ser::IsArchiveT Archive>
    void serialize(Archive& ar) {
        ar.property("yawn", yawn);
        ar.property("pitch", pitch);
        ar.property("roll", roll);
    }
};

struct Transform {
    Position position {};
    Rotation rotation {};

    template <ser::IsArchiveT Archive>
    void serialize(Archive& ar) {
        ar.property("position", position);
        ar.property("rotation", rotation);
    }
};

struct State {
    int version { 0 };
    std::string name {};
    std::vector<Transform> transforms {};

    template <ser::IsArchiveT ArchiveT>
    void serialize(ArchiveT& ar) {
        ar.property("version", version);
        ar.property("name", name);
        ar.property("transforms", transforms);
    }
};

int main() {
    ser::DebugStdoutWriter debug_stdout_writer({ .tab_width = 4, .print_types = true });

    Position position_a { .x = 1.0f, .y = -1.0f, .z = 42.42f };
    Rotation rotation_a { .yawn = 1.0f, .pitch = 0.5f, .roll = 0.69f };
    Transform transform_a { .position = position_a, .rotation = rotation_a };

    Position position_b { .x = 67.67f, .y = -42.0f, .z = 42.42f };
    Rotation rotation_b { .yawn = 3.3f, .pitch = 1.5f, .roll = 42.69f };
    Transform transform_b { .position = position_b, .rotation = rotation_b };

    State state {
        .version = 0,
        .name    = "Game of Life",
        .transforms { transform_a, transform_b },
    };

    state.serialize(debug_stdout_writer);

    return 0;
}
