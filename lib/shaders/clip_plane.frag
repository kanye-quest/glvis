#ifdef __EMSCRIPTEN__
R"(
uniform bool useClipPlane;
varying float fClipVal;

void fragmentClipPlane() {
    if (useClipPlane && fClipVal < 0.0) {
        discard;
    }
}
)"
#else
R"(
void fragmentClipPlane() {
}
")
#endif
