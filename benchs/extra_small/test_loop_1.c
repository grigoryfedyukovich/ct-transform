#include "../smack.h"
#include "../ct-verif.h"

int fn(){
  unsigned int s = 0;
  while (s < 60)
  {
    if (s == 17) break;
    s++;
  }

  return 0;
}
