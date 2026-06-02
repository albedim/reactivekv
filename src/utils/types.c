int isInt(const char* str) {
  if (*str == '-' || *str == '+') str++;

  if (*str == '\0') return 0;

  while (*str != '\0') {
      if (!isdigit(*str)) return 0;
      str++;
  }
  return 1;
}

int isFloat(const char* str) {
  if (*str == '-' || *str == '+') str++;

  if (*str == '\0') return 0;

  int dotCount = 0;
  while (*str != '\0') {
      if (*str == '.') {
        dotCount++;
        if (dotCount > 1) return 0;
      } else if (!isdigit(*str)) {
          return 0;
      }
      str++;
  }
  return 1;
}