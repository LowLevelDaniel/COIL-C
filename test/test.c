/**
 * Test file for the COIL C Compiler
 * This file exercises various C language features
 * that should be supported by the compiler.
 */

// Function to add two integers
int add(int a, int b) {
  return a + b;
}

// Function to calculate factorial recursively
int factorial(int n) {
  if (n <= 1) {
      return 1;
  }
  return n * factorial(n - 1);
}

// Function with multiple parameters and local variables
int calculate(int a, int b, int operation) {
  int result;
  
  if (operation == 0) {
      // Addition
      result = a + b;
  } else if (operation == 1) {
      // Subtraction
      result = a - b;
  } else if (operation == 2) {
      // Multiplication
      result = a * b;
  } else if (operation == 3 && b != 0) {
      // Division (avoid divide by zero)
      result = a / b;
  } else {
      // Default to addition
      result = a + b;
  }
  
  return result;
}

// Function with a while loop
int sum_to_n(int n) {
  int sum = 0;
  int i = 1;
  
  while (i <= n) {
      sum = sum + i;
      i = i + 1;
  }
  
  return sum;
}

// Function with a for loop
int sum_squares(int n) {
  int sum = 0;
  
  for (int i = 1; i <= n; i = i + 1) {
      sum = sum + i * i;
  }
  
  return sum;
}

// Main function
int main() {
  // Simple variable declarations and initializations
  int x = 10;
  int y = 5;
  int z;
  
  // Function calls
  z = add(x, y);
  
  // Assignment with expression
  x = x + 1;
  
  // Function call with complex expression
  int result = calculate(x, y, 2);
  
  // Loop example
  int sum = 0;
  for (int i = 0; i < 5; i = i + 1) {
      sum = sum + i;
  }
  
  // Conditional example
  if (sum > 10) {
      sum = 10;
  } else {
      sum = sum * 2;
  }
  
  // Return the final result
  return result;
}