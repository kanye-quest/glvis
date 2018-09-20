#ifdef __EMSCRIPTEN__
R"(
varying float fClipVal;

void setupClipPlane(in float dist) {
    fClipVal = dist;
}
)"
#else
R"(
void setupClipPlane(in float dist) {
    gl_ClipDistance[0] = dist;
}
)"
#endif
