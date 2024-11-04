#ifndef MAIN_H
#define MAIN_H
#include "return_codes.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int hex_to_uint32(const char *hex, uint32_t *result)
{
	if (strlen(hex) < 3 || hex[0] != '0' || hex[1] != 'x')
	{
		return ERROR_DATA_INVALID;
	}
	*result = 0;
	for (int i = 2; hex[i] != '\0'; i++)
	{
		*result <<= 4;
		if (hex[i] >= '0' && hex[i] <= '9')
		{
			*result |= (hex[i] - '0');
		}
		else if (hex[i] >= 'A' && hex[i] <= 'F')
		{
			*result |= (hex[i] - 'A' + 10);
		}
		else if (hex[i] >= 'a' && hex[i] <= 'f')
		{
			*result |= (hex[i] - 'a' + 10);
		}
		else
		{
			return ERROR_ARGUMENTS_INVALID;
		}
	}
	return SUCCESS;
}

int parse_arguments(int argc, char *argv[], _Bool *single, uint8_t *rounding_mode, uint32_t *num1, char *operation, uint32_t *num2)
{
	if ((argc != 4 && argc != 6) || strlen(argv[1]) != 1)
	{
		return ERROR_ARGUMENTS_INVALID;
	}

	switch (argv[1][0])
	{
	case 'h':
		*single = 0;
		break;
	case 'f':
		*single = 1;
		break;
	default:
		return ERROR_ARGUMENTS_INVALID;
	}

	*rounding_mode = argv[2][0] - '0';
	if (*rounding_mode < 0 || *rounding_mode > 3 || strlen(argv[2]) != 1 || strlen(argv[3]) > 10 ||
		(hex_to_uint32(argv[3], num1) != SUCCESS))
	{
		return ERROR_ARGUMENTS_INVALID;
	}

	if (argc == 6)
	{
		if ((argv[4][0] != '+' && argv[4][0] != '-' && argv[4][0] != '*' && argv[4][0] != '/') ||
			strlen(argv[4]) != 1 || (hex_to_uint32(argv[5], num2) != SUCCESS))
		{
			return ERROR_ARGUMENTS_INVALID;
		}
		*operation = argv[4][0];
	}
	else
	{
		*operation = '\0';
	}

	return SUCCESS;
}

uint8_t number_the_first_bit(uint64_t mantissa)
{
	if (mantissa == 0)
		return 0;
	uint8_t pos = 0;
	while (mantissa >>= 1)
	{
		pos++;
	}
	return pos;
}

void adjust_mantissa(uint64_t *mantissa, int16_t *exponent, _Bool single, uint8_t size_mant)
{
	int8_t shift_amount = number_the_first_bit(*mantissa) - size_mant;
	if (shift_amount <= 0)
		return;
	*mantissa >>= shift_amount;
	*exponent += shift_amount;
}

void convert_mantissa_to_hex(uint32_t mantissa, _Bool single)
{
	uint32_t mask = single ? 0x7FFFFF : 0x3FF;
	uint32_t len = single ? 6 : 3;
	uint32_t dif = single ? 23 : 10;
	uint32_t fractional_part = mantissa & mask;

	for (int i = 0; i < len; i++)
	{
		fractional_part <<= 4;
		uint32_t hex_digit = (fractional_part >> dif) & 0xF;
		printf("%x", hex_digit);
	}

	printf("p");
}

void set_components(uint32_t num, _Bool *sign, int16_t *exponent, uint32_t *mantissa, _Bool single)
{
	uint8_t s_dif = single ? 31 : 15;
	uint8_t exp_dif = single ? 23 : 10;
	uint32_t exp_max = single ? 0xFF : 0x1F;
	uint32_t mask = single ? 0x7FFFFF : 0x3FF;
	*sign = num >> s_dif;
	*exponent = (num >> exp_dif) & exp_max;
	*mantissa = num & mask;
	if (*exponent != 0 && *exponent != exp_max)
	{
		*mantissa = single ? *mantissa | 0x800000 : *mantissa | 0x400;
	}
}

int rounding(uint64_t *mantissa, int16_t *exponent, uint8_t rounding_mode, _Bool sign, char operation, _Bool single, uint64_t fix_mantissa)
{
	uint8_t size_mant = single ? 23 : 10;
	uint8_t mant_dif = single ? (operation == '*' || operation == 'M' ? 12 : 6) : (operation == '*' || operation == 'M' ? 6 : 3);

	uint8_t num_first_bit = number_the_first_bit(*mantissa);

	uint8_t delta = num_first_bit > size_mant + mant_dif ? num_first_bit - size_mant : mant_dif;

	fix_mantissa = *mantissa << 64 - delta >> 64 - delta;
	*mantissa = *mantissa >> delta;
	*exponent = *exponent + delta - mant_dif;

	uint32_t bit_dif_bit = 1 << mant_dif;

	switch (rounding_mode)
	{
	case 0:
		break;
	case 1:
		if (fix_mantissa == (1 << mant_dif))
		{
			if (*mantissa & 0x1)
			{
				*mantissa += 1;
			}
		}
		else if (fix_mantissa > bit_dif_bit)
		{
			if (sign == 0)
			{
				*mantissa += 1;
			}
			else
			{
				*mantissa -= 1;
			}
		}
		break;
	case 2:
	case 3:
		*mantissa =
			((fix_mantissa > 0 && sign == 0 && rounding_mode == 2) || (fix_mantissa > 0 && sign == 1 && rounding_mode == 3))
				? *mantissa + 1
				: *mantissa;
		break;
	default:
		return ERROR_ARGUMENTS_INVALID;
	}
	return SUCCESS;
}

int perform_operation(uint32_t num1, char operation, uint32_t num2, uint32_t *result, int rounding_mode, _Bool single)
{
	_Bool sign1, sign2;
	int16_t exponent1 = 0, exponent2 = 0;
	uint32_t mantissa1 = 0, mantissa2 = 0;

	set_components(num1, &sign1, &exponent1, &mantissa1, single);
	if (operation != '\0')
	{
		set_components(num2, &sign2, &exponent2, &mantissa2, single);
	}

	uint8_t mant_dif = single ? 6 : 3;
	uint8_t size_mant = single ? 23 : 10;
	uint8_t exp_minus = single ? 127 : 15;
	uint8_t max_exp = single ? 0xFF : 0x1F;
	uint8_t move_sign = single ? 31 : 15;
	uint32_t mask_mantissa = single ? 0x7FFFFF : 0x3FF;
	mantissa1 <<= mant_dif;
	mantissa2 <<= mant_dif;

	_Bool res_sign = 0;
	int16_t res_exponent = 0;
	uint64_t res_mantissa = 0;
	uint64_t fix_mantissa = 0;

	switch (operation)
	{
	case '+':
	case '-':
	{
		if (operation == '-')
		{
			sign2 ^= 1;
		}

		int32_t exp_diff = exponent1 - exponent2;
		if (exp_diff > 0)
		{
			mantissa2 = (exp_diff > size_mant) ? 0 : mantissa2 >> exp_diff;
			res_exponent = exponent1;
		}
		else if (exp_diff < 0)
		{
			mantissa1 = (-exp_diff > size_mant) ? 0 : mantissa1 >> -exp_diff;
			res_exponent = exponent2;
		}
		else
		{
			res_exponent = exponent1;
		}
		if (sign1 == sign2)
		{
			res_mantissa = (uint64_t)mantissa1 + (uint64_t)mantissa2;
			res_sign = sign1;
		}
		else
		{
			if (mantissa1 >= mantissa2)
			{
				res_mantissa = (uint64_t)mantissa1 - (uint64_t)mantissa2;
				res_sign = sign1;
			}
			else
			{
				res_mantissa = (uint64_t)mantissa2 - (uint64_t)mantissa1;
				res_sign = sign2;
			}
		}
		break;
	}
	case '*':
	{
		res_sign = sign1 ^ sign2;
		res_exponent = exponent1 + exponent2 - exp_minus;
		res_mantissa = ((uint64_t)mantissa1 * (uint64_t)mantissa2) >> size_mant;
		break;
	}
	case '/':
	{
		// проверка на nan (одно из чисел nan)
		if ((exponent1 == max_exp && mantissa1 != 0) || (exponent2 == max_exp && mantissa2 != 0))
		{
			// возвращаем nan
			*result = (res_sign << move_sign) | (res_exponent << size_mant) | 0;
			goto exit_label;
		}
		// проверак на inf ( первое число inf )
		if (exponent1 == max_exp && mantissa1 == 0)
		{
			// второе число не inf -> вернуть inf
			if ((exponent2 != max_exp || mantissa2 != 0))
				*result = (sign1 ^ sign2) << move_sign | max_exp << size_mant | 0;
			// все же inf -> вернуmь nan
			else
				*result = (res_sign << move_sign) | (max_exp << size_mant) | 1;
			goto exit_label;
		}
		if (exponent2 == max_exp && mantissa2 == 0)
		{
			*result = 0;
			goto exit_label;
		}
		// прверка на 0
		if (exponent2 == 0 && mantissa2 == 0)
		{
			*result = (sign1 ^ sign2) << move_sign | max_exp << size_mant | res_mantissa;
			goto exit_label;
		}
		// деление 2х чисел
		res_sign = sign1 ^ sign2;
		res_exponent = exponent1 - exponent2 + exp_minus;
		uint64_t dividend = ((uint64_t)mantissa1) << size_mant;
		res_mantissa = dividend / (mantissa2 >> mant_dif);
		break;
	}
	case '\0':
	{
		*result = num1;
		return SUCCESS;
	}
	default:
		return ERROR_ARGUMENTS_INVALID;
	}

	if (rounding(&res_mantissa, &res_exponent, rounding_mode, res_sign, operation, single, fix_mantissa) != SUCCESS)
	{
		return ERROR_ARGUMENTS_INVALID;
	}

	adjust_mantissa(&res_mantissa, &res_exponent, single, size_mant);

	while ((res_mantissa < (1 << size_mant)) && res_mantissa != 0 && res_exponent != 0)
	{
		res_mantissa <<= 1;
		res_exponent--;
	}

	*result = (res_sign << move_sign) | (res_exponent << size_mant) | (res_mantissa & mask_mantissa);
exit_label:
	return SUCCESS;
}

void normaliz(int16_t *exponent, uint32_t *mantissa, _Bool single)
{
	uint8_t size_mant = single ? 23 : 10;
	if (*exponent == 0 && *mantissa != 0)
	{
		while (*mantissa < (1 << size_mant))
		{
			*mantissa <<= 1;
			*exponent -= 1;
		}
		*exponent += 1;
	}
}

_Bool zero(uint32_t mantissa, uint8_t exponent, _Bool single)
{
	if (exponent == 0 && mantissa == 0)
	{
		single ? printf("0x0.000000p+0") : printf("0x0.000p+0");
		return 1;
	}
	return 0;
}

_Bool inf_nan(uint32_t mantissa, uint8_t exponent, _Bool sign, _Bool single)
{
	if ((exponent == 0xFF && single) || (exponent == 0x1F && !single))
	{
		if (mantissa == 0)
		{
			sign ? printf("-inf") : printf("inf");
		}
		else
		{
			printf("nan");
		}
		return 1;
	}
	return 0;
}

void print_result(uint32_t result, _Bool single)
{
	_Bool sign = 0;
	int16_t exponent = 0;
	uint32_t mantissa = 0;
	// result разваливается на части
	set_components(result, &sign, &exponent, &mantissa, single);

	normaliz(&exponent, &mantissa, single);
	if (zero(mantissa, exponent, single))
		return;
	if (inf_nan(mantissa, exponent, sign, single))
		return;

	exponent = single ? exponent - 127 : exponent - 15;

print:
	if (sign == 1)
		printf("-");
	(mantissa == 0 && exponent == 0) ? printf("0x0.") : printf("0x1.");
	convert_mantissa_to_hex(mantissa, single);
	printf("%+d", exponent);
}

#endif
