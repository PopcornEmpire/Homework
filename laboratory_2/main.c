#include "audio_processing.h"
#include "cross_correlation.h"
#include "return_codes.h"
#include <libavutil/log.h>

#include <stdio.h>
#include <stdlib.h>

#define CLEANUP(resource, free_function)                                                                               \
	if (resource)                                                                                                      \
	{                                                                                                                  \
		free_function(resource);                                                                                       \
	}

//* все комментарии касающиеся исправленных ошибок пометил как //*

//* теперь статичны
static int initialize_channel(AudioData *channel, const AudioData *source_audio, const char *channel_name);
static int
	initialize_two_channels(AudioData *channel1, AudioData *channel2, const AudioData *source1, const AudioData *source2, const char *name1, const char *name2);
static void copy_channel_data(const AudioData *source, AudioData *channel);

int main(int argc, char *argv[])
{
	if (argc < 2 || argc > 3)
	{
		fprintf(stderr, "Usage: %s <file1> [<file2>]\n", argv[0]);
		return ERROR_ARGUMENTS_INVALID;
	}

	const char *file1 = argv[1];
	const char *file2 = (argc == 3) ? argv[2] : NULL;

	AudioData audio_data1 = { 0 };
	AudioData audio_data2 = { 0 };
	int result = SUCCESS;
	// отключает часть вывода AV
	av_log_set_level(AV_LOG_ERROR);

	// используются для копирования массива сэмплов из audio_data1/2. Не очень нужно, но позволяет вынести общий код
	AudioData channel1 = { 0 }, channel2 = { 0 };

	result = read_audio_file(file1, &audio_data1);
	if (result != SUCCESS)
	{
		fprintf(stderr, "Error reading audio file: %s\n", file1);
		goto cleanup;
	}

	// условие: 1 файл - 2 канала или 2 файла - беру первый канал из каждого
	if (file2 == NULL)
	{
		if (audio_data1.channels != 2)
		{
			fprintf(stderr, "Audio file does not have 2 channels\n");
			result = ERROR_FORMAT_INVALID;
			goto cleanup;
		}

		// инициализирую левый и правый канал
		result = initialize_two_channels(&channel1, &channel2, &audio_data1, &audio_data1, "left", "right");
		if (result != SUCCESS)
		{
			goto cleanup;
		}
		//* использую size_t
		for (size_t i = 0; i < channel1.sample_count; i++)
		{
			channel1.samples[i] = audio_data1.samples[audio_data1.channels * i];
			channel2.samples[i] = audio_data1.samples[audio_data1.channels * i + 1];
		}
	}
	else
	{
		result = read_audio_file(file2, &audio_data2);
		if (result != SUCCESS)
		{
			fprintf(stderr, "Error reading audio file: %s\n", file2);
			goto cleanup;
		}

		if (audio_data1.sample_rate != audio_data2.sample_rate)
		{
			fprintf(stderr, "Different sample rates are not supported.\n");
			result = ERROR_UNSUPPORTED;
			goto cleanup;
		}

		// взять по первому каналу из каждого файла
		result = initialize_two_channels(&channel1, &channel2, &audio_data1, &audio_data2, "file1", "file2");
		if (result != SUCCESS)
			goto cleanup;

		// Копирование данных из первого и второго файла
		copy_channel_data(&audio_data1, &channel1);
		copy_channel_data(&audio_data2, &channel2);
	}

	int delta_samples;
	result = calculate_cross_correlation(&channel1, &channel2, &delta_samples);
	if (result != SUCCESS)
	{
		fprintf(stderr, "Error calculating cross-correlation\n");
		goto cleanup;
	}

	//* теперь считаю, как было указано в комментарии
	int delta_time_ms = (int)((double)(delta_samples * 1000) / channel1.sample_rate);
	printf("delta: %i samples\nsample rate: %i Hz\ndelta time: %i ms\n", delta_samples, channel1.sample_rate, delta_time_ms);
	result = SUCCESS;
cleanup:
	CLEANUP(&audio_data1, delete_audio_data);
	CLEANUP(&audio_data2, delete_audio_data);
	CLEANUP(&channel1, delete_audio_data);
	CLEANUP(&channel2, delete_audio_data);

	return result;
}

static int initialize_channel(AudioData *channel, const AudioData *source_audio, const char *channel_name)
{
	channel->sample_rate = source_audio->sample_rate;
	channel->channels = 1;
	channel->sample_count = source_audio->sample_count / source_audio->channels;

	channel->samples = malloc(sizeof(int16_t) * channel->sample_count);
	if (!channel->samples)
	{
		fprintf(stderr, "Memory allocation failed for %s channel\n", channel_name);
		return ERROR_NOTENOUGH_MEMORY;
	}

	return SUCCESS;
}

static int
	initialize_two_channels(AudioData *channel1, AudioData *channel2, const AudioData *source1, const AudioData *source2, const char *name1, const char *name2)
{
	int result = initialize_channel(channel1, source1, name1);
	if (result != SUCCESS)
	{
		return result;
	}

	result = initialize_channel(channel2, source2, name2);
	if (result != SUCCESS)
	{
		return result;
	}

	return SUCCESS;
}

static void copy_channel_data(const AudioData *source, AudioData *channel)
{
	for (size_t i = 0; i < channel->sample_count; i++)
	{
		channel->samples[i] = source->samples[source->channels * i];
	}
}
