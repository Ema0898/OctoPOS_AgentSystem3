#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int flatten_int(int data, char *target, int targetOffset)
{
  if (data == 0)
  {
    target[targetOffset] = '0';
    target[targetOffset + 1] = ',';
    return 1;
  }

  int temp = data;
  int power = 1;

  while (power < temp)
    power *= 10;

  power /= 10;

  int digit = 0;
  int i = 0;

  while (power)
  {
    digit = data / power;
    target[i + targetOffset] = digit + '0';
    data %= power;
    power /= 10;
    i++;
  }

  target[targetOffset + i] = ',';

  return i;
}

int main()
{

  // int hola = 54542545;
  // int hola2 = 23;
  // int hola3 = 34;
  // char *adios = malloc(10 * sizeof(char));

  // int offset = flatten_int(hola, adios, 0);
  // int offset2 = flatten_int(hola2, adios, offset + 1);
  // int offset3 = flatten_int(hola3, adios, offset + offset2 + 2);
  // adios[offset + offset2 + offset3 + 2] = '\0';
  // printf("Flattened char = %s\n", adios);
  char *adios = malloc(100 * sizeof(char));

  int hola1 = 350;
  int hola2 = 0;
  int hola3 = 0;

  int offset1 = flatten_int(hola1, adios, 0);
  int offset2 = flatten_int(hola2, adios, offset1 + 1);

  char *adios2 = "HOLAAAAAL";

  memcpy(adios + offset1 + offset2 + 2, adios2, strlen(adios2));
  // int offset3 = flatten_int(hola3, adios, offset1 + offset2 + 2);

  // int nums[] = {11, 200, 3000, 400, 50};

  // int i = 0;
  // int offsetFor = offset1 + offset2 + offset3 + 3;

  // for (i = 0; i < 5; ++i)
  // {
  //   printf("offset = %d\n", offsetFor);
  //   offsetFor += flatten_int(nums[i], adios, offsetFor + i);
  // }

  // printf("i = %d\n", i);
  // adios[offsetFor + i - 1] = '\0';
  printf("Flattened char = %s\n", adios);

  // int hola2 = 20;
  // char adios2 = hola2 + '0';

  // printf("ADIOS2 = %c\n", adios2);
  // char text[] = "StringX";
  // int digit;
  // for (digit = 0; digit < 10; ++digit)
  // {
  //   text[6] = digit + '0';
  //   puts(text);
  // }

  return 0;
}