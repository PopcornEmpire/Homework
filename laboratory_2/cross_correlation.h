#ifndef CROSS_CORRELATION_H
#define CROSS_CORRELATION_H

#include "return_codes.h"

#include <fftw3.h>
#include <stdio.h>
#include <string.h>

#define CLEANUP(resource, free_function)                                                                               \
	if (resource)                                                                                                      \
	{                                                                                                                  \
		free_function(resource);                                                                                       \
	}

//* теперь size_t
//* функция статичная
static void initialize_fftw_input(fftw_complex *in, const AudioData *audio, size_t padded_N)
{
	memset(in, 0, padded_N * sizeof(fftw_complex));
	for (size_t i = 0; i < audio->sample_count; i++)
	{
		in[i][0] = (double)audio->samples[i];
	}
}

int calculate_cross_correlation(const AudioData *audio1, const AudioData *audio2, int *delta_samples)
{
	int8_t result = SUCCESS;
	if (audio1->sample_rate != audio2->sample_rate)
	{
		fprintf(stderr, "Sample rates do not match\n");
		return ERROR_UNSUPPORTED;
	}
	//* теперь size_t
	size_t N = audio1->sample_count + audio2->sample_count - 1;
	size_t power = 1;
	while (power < N)
	{
		power <<= 1;
	}
	//* теперь одна аллокация
	fftw_complex *memory_block = fftw_malloc(sizeof(fftw_complex) * power * 5);
	if (!memory_block)
	{
		fprintf(stderr, "Could not allocate memory for FFT\n");
		result = ERROR_NOTENOUGH_MEMORY;
		goto cleanup;
	}

	fftw_complex *in1 = memory_block;
	fftw_complex *in2 = memory_block + power;
	fftw_complex *out1 = memory_block + 2 * power;
	fftw_complex *out2 = memory_block + 3 * power;
	fftw_complex *cross_corr = memory_block + 4 * power;

	initialize_fftw_input(in1, audio1, power);
	initialize_fftw_input(in2, audio2, power);

	fftw_plan p1 = fftw_plan_dft_1d(power, in1, out1, FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_plan p2 = fftw_plan_dft_1d(power, in2, out2, FFTW_FORWARD, FFTW_ESTIMATE);

	if (!p1 || !p2)
	{
		fprintf(stderr, "FFTW plan creation failed\n");
		result = ERROR_DATA_INVALID;
		goto cleanup;
	}

	fftw_execute(p1);
	fftw_execute(p2);

	for (size_t i = 0; i < power; i++)
	{
		cross_corr[i][0] = out1[i][0] * out2[i][0] + out1[i][1] * out2[i][1];
		cross_corr[i][1] = out1[i][1] * out2[i][0] - out1[i][0] * out2[i][1];
	}

	fftw_plan p3 = fftw_plan_dft_1d(power, cross_corr, cross_corr, FFTW_BACKWARD, FFTW_ESTIMATE);
	if (!p3)
	{
		fprintf(stderr, "FFTW inverse plan creation failed\n");
		result = ERROR_DATA_INVALID;
		goto cleanup;
	}

	fftw_execute(p3);

	double max_value = cross_corr[0][0] / power;
	size_t max_index = 0;
	//* теперь size_t
	for (size_t i = 1; i < power; i++)
	{
		double val = cross_corr[i][0] / power;
		if (val > max_value)
		{
			max_value = val;
			max_index = i;
		}
	}

	*delta_samples = (int)(max_index > power / 2 ? max_index - power : max_index);
	result = SUCCESS;
//* теперь есть метка к которой я сразу перехожу в случае ошибки
cleanup:
	CLEANUP(p1, fftw_destroy_plan);
	CLEANUP(p2, fftw_destroy_plan);
	CLEANUP(p3, fftw_destroy_plan);
	CLEANUP(memory_block, fftw_free);
	return result;
}

#endif
