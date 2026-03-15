#include "../../include/serialization.hpp"

struct Position {
    float x {};
    float y {};
    float z {};

    template <ser::IsArchiveT Archive>
    void serialize(Archive& ar) {
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

int main() {
    ser::DebugWriter debug_writer;
    Position position { .x = 1.0f, .y = -1.0f, .z = 42.42f };
    Rotation rotation { .yawn = 1.0f, .pitch = 0.5f, .roll = 0.69f };
    Transform transform { .position = position, .rotation = rotation };

    transform.serialize(debug_writer);

    return 0;
}
