/*
  Tests for function calls and variable access

  After redact:
   f0(0) => 5
   f0(1) => crash
   f1(10) => -1
   f1(5) => crash
   f1(1) => 1
   f1(0) => 5
   f2(0) => 0
   f2(1) => 2
   f3(0) => 5
   f3(1) => crash
   f3(2) => 2
   g0(0) => 3
   g0(1) => crash
   g1(1) => -1
   g1(0) => crash
*/

int a1 = 1, a2 = 2;

int f0(int sel) {
  if (sel)
    return a1;
  else
    return 5;
}

int f1(int sel) {
  if (sel >= 10)
    return -1;
  else if (sel >= 5)
    return a2;
  else if (sel > 0)
    return a1;
  else
    return f0(0);
}

int f2(int sel) {
  if (sel)
    return a2;
  else
    return 0;
}

int f3(int sel) {
  if (sel < 2)
    return f0(sel);
  else
    return f2(sel);
}

int g0(int sel) {
  if (sel)
    return f1(sel);
  else
    return 3;
}

int g1(int sel) {
  if (sel)
    f1(11);
  else
    f3(3);
}
