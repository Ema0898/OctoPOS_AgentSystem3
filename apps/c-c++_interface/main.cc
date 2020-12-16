#include <iostream>

using namespace std;

typedef void *example_t;

class MyClass
{
public:
  int myNum;
  string myString;
};

void hola(example_t ex);
example_t adios();

/* C part */
int main()
{
  example_t temp = adios();

  hola(temp);

  return 0;
}

/* C++ part */
void hola(example_t ex)
{
  MyClass *temp = static_cast<MyClass *>(ex);

  cout << "Num: " << temp->myNum << endl;
  cout << "String: " << temp->myString << endl;
}

example_t adios()
{
  MyClass *temp = new MyClass();

  temp->myNum = 11;
  temp->myString = "HOLA MUNDO";

  return temp;
}