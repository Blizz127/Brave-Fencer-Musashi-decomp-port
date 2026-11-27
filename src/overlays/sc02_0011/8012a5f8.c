/* Apply a new field view with the requested transition duration. */
extern void func_8012A62C(unsigned char duration);

void func_8012A5F8(void (*update)(void), unsigned char duration)
{
    func_8012A62C(duration);
    update();
}
