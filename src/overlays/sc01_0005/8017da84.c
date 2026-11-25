/* Pick the room's music using the signed story value. */
int func_8017DA84(int story)
{
    if (story < 5) {
        return 0x57;
    }
    return 0x3D;
}
