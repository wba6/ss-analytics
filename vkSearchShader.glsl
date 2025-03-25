#version 450
layout(local_size_x = 256) in;

// Buffer holding the text (each element is an ASCII code).
layout(std430, binding = 0) buffer TextBuffer {
    uint text[];
};
// Buffer holding the pattern.
layout(std430, binding = 1) buffer PatternBuffer {
    uint pattern[];
};
// Buffer to store the output: the lowest matching index.
layout(std430, binding = 2) buffer ResultBuffer {
    int matchIndex;
};

// Push constants for the lengths of the text and the pattern.
layout(push_constant) uniform PushConstants {
    uint textLength;
    uint patternLength;
} pc;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx > pc.textLength - pc.patternLength) return;
    bool found = true;
    for (uint i = 0; i < pc.patternLength; i++) {
        if (text[idx + i] != pattern[i]) {
            found = false;
            break;
        }
    }
    if (found) {
        atomicMin(matchIndex, int(idx));
    }
}


