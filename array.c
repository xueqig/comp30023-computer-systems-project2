#include <stdio.h>

void modify(char **s)
{
    // char *new_name = "Australia";
    *s = "Australia";
}

int main()
{
    char *name = "New Holland";
    modify(&name);
    printf("%s\n", name);
    return 0;
}