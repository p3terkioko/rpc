#ifndef CALCULATOR_OPS_H
#define CALCULATOR_OPS_H

// Structure to hold the result of a calculation
typedef struct {
    double value;
    char error[256]; // To store error messages, e.g., "Division by zero"
} CalcResult;

// Function prototypes for calculator operations
CalcResult add(double a, double b);
CalcResult subtract(double a, double b);
CalcResult multiply(double a, double b);
CalcResult divide(double a, double b);

#endif // CALCULATOR_OPS_H
