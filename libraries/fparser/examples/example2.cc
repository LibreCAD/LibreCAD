// Simple example file for the function parser
// ===========================================
/* Note that the library has to be compiled with
   FP_SUPPORT_FLOAT_TYPE, FP_SUPPORT_LONG_DOUBLE_TYPE and
   FP_SUPPORT_LONG_INT_TYPE
   preprocessor macros defined for this example to work.

   Try with these input values with the different floating point parser
   types to see the difference in accuracy:

f(x) = x + 1.234567890123456789
min x: 0
max x: 2
step: 1
*/

#include "../fparser.hh"

#include <iostream>
#include <iomanip>
#include <string>

template<typename Parser>
void runExample(const char* valueTypeName)
{
    typedef typename Parser::value_type Value_t;

    std::cout << "Using " << valueTypeName << " parser." << std::endl;

    Parser fparser;
    std::string function;
    Value_t minx, maxx, step;

    fparser.AddConstant("pi", Value_t(3.1415926535897932));

    std::cin.ignore();
    while(true)
    {
        std::cout << "f(x) = ";
        std::getline(std::cin, function);
        if(std::cin.fail()) return;

        int res = fparser.Parse(function, "x");
        if(res < 0) break;

        std::cout << std::string(res+7, ' ') << "^\n"
                  << fparser.ErrorMsg() << "\n\n";
    }

    std::cout << "min x: ";
    std::cin >> minx;
    std::cout << "max x: ";
    std::cin >> maxx;
    std::cout << "step: ";
    std::cin >> step;
    if(std::cin.fail()) return;

    Value_t vals[] = { 0 };
    for(vals[0] = minx; vals[0] <= maxx; vals[0] += step)
    {
        std::cout << std::setprecision(20);
        std::cout << "f(" << vals[0] << ") = " << fparser.Eval(vals)
                  << std::endl;
    }
}

int main()
{
    int choice = 0;
    do
    {
        std::cout << "1 = double, 2 = float, 3 = long double, 4 = long int: ";
        std::cin >> choice;
    } while(choice < 1 || choice > 4);

    switch(choice)
    {
      case 1: runExample<FunctionParser>("double"); break;
      case 2: runExample<FunctionParser_f>("float"); break;
      case 3: runExample<FunctionParser_ld>("long double"); break;
      case 4: runExample<FunctionParser_li>("long int"); break;
    }

    return 0;
}
