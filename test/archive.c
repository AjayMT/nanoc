
extern int foo;

void _start();

int main(int a, int b, int c)
{
  foo = 1;
  _start();
  return (int)_start;
}
