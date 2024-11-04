#include "main.h"

#include "return_codes.h"

#include <stdint.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	_Bool single;
	uint8_t rounding_mode;
	uint32_t num1 = 0, num2 = 0, result = 0;
	char operation = '\0';

	if (parse_arguments(argc, argv, &single, &rounding_mode, &num1, &operation, &num2) != SUCCESS)
	{
		fprintf(stderr, "Error: invalid arguments\n");
		return ERROR_ARGUMENTS_INVALID;
	}
	if (perform_operation(num1, operation, num2, &result, rounding_mode, single) != SUCCESS)
	{
		fprintf(stderr, "Error: operation failed\n");
		return ERROR_DATA_INVALID;
	}
	print_result(result, single);

	return SUCCESS;
}
