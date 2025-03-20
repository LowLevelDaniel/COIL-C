/**
 * Sample C program to demonstrate COIL compilation
 */

// Simple function to calculate factorial
int factorial(int n) {
  if (n <= 1) {
    return 1;
  } else {
    return n * factorial(n - 1);
  }
}

// Function to calculate Fibonacci numbers
int fibonacci(int n) {
  if (n <= 0) {
    return 0;
  } else if (n == 1) {
    return 1;
  } else {
    return fibonacci(n - 1) + fibonacci(n - 2);
  }
}

// Function to swap two integers using pointers
void swap(int* a, int* b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

// Main function
int main() {
  int a = 5;
  int b = 10;
  
  // Calculate and print factorial
  int fact = factorial(a);
  
  // Calculate and print Fibonacci
  int fib = fibonacci(a);
  
  // Swap a and b
  swap(&a, &b);
  
  // Return sum of factorial and Fibonacci as exit code
  return fact + fib;
}