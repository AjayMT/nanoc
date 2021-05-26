int foo();

int main(int a, int b, int c)
{
  b = foo() + a;
  return (int)foo;
}

int foo()
{
  return 0;
}
