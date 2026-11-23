/* Increase the player's maximum HP, with the game's limit of 500. */
extern unsigned short D_80078EB2;

void func_8014BC0C(int unused, unsigned int amount)
{
    D_80078EB2 += amount;
    if (D_80078EB2 > 500) {
        D_80078EB2 = 500;
    }
}
