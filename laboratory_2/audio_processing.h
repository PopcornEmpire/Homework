#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H

#include "return_codes.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
	int sample_rate;
	int channels;
	//* теперь size_t
	size_t sample_count;
	int16_t *samples;
} AudioData;

#define CLEANUP(resource, free_function)                                                                               \
	if (resource)                                                                                                      \
	{                                                                                                                  \
		free_function(&resource);                                                                                      \
	}

// статичная
static int get_error_code(int av_error_code)
{
	// просто переписано сапоставление из error.h
	switch (av_error_code)
	{
	case AVERROR(ENOENT):
		return ERROR_CANNOT_OPEN_FILE;
	case AVERROR(ENOMEM):
		return ERROR_NOTENOUGH_MEMORY;
	case AVERROR_INVALIDDATA:
		return ERROR_DATA_INVALID;
	case AVERROR_EOF:
		return ERROR_DATA_INVALID;
	case AVERROR_EXPERIMENTAL:
		return ERROR_UNSUPPORTED;
	case AVERROR_INPUT_CHANGED:
		return ERROR_DATA_INVALID;
	case AVERROR_OUTPUT_CHANGED:
		return ERROR_DATA_INVALID;
	default:
		// под большинство ошибок
		return ERROR_UNKNOWN;
	}
}

// Суть функции read_audio_file:
// открыть файл ->
// настроить декодер ->
// настроить ресемплинг (привести все данные к int16_t) ->
// чтение и декодирование ->
// заполнение стурктуры ->
// освобождение рессурсов
// в итоге получаем данные в том числе массив семлов, которые будут использоватся для кросс корреляции

// Структура файла:
// Файл ->
// Поток ->
// Пакет ->
// Фрейм ->
// Канал ->
// Семплы ->

// Сквозной смысл:
// инициализиваровать контекст формата файла -> найти информацию о потоках -> найти индекс потока ->
//	взять поток -> найти декодер -> создание нового контекста, связанного с выбранным декодером ->
// передача параметров кодека из потока в контекст -> открыть кодек ->
// проверка раскладки каналов -> создание и настройка контекста для ресемплинга ->
// инициализация ресемплера -> отправка пакета на декодирование -> извлекаю фреймы ->
// преобразование в нужный формат семплов -> окончаетльно перести данные в структуру AudioData ->
// очситить ресурсы

int read_audio_file(const char *file_name, AudioData *audio_data)
{
	AVFormatContext *format_context = NULL;
	AVCodecContext *codec_context = NULL;
	AVPacket *audio_packet = NULL;
	AVFrame *audio_frame = NULL;
	SwrContext *resample_context = NULL;
	uint8_t **resampled_data = NULL;
	int resampled_data_size = 0;
	int result_code = 0;

	audio_packet = av_packet_alloc();
	audio_frame = av_frame_alloc();
	if (!audio_packet || !audio_frame)
	{
		fprintf(stderr, "Failed to allocate memory for packet or frame\n");
		result_code = ERROR_NOTENOUGH_MEMORY;
		goto cleanup;
	}

	// инициализиваровать контекст формата файла
	result_code = avformat_open_input(&format_context, file_name, NULL, NULL);
	if (result_code < 0)
	{
		//* теперь обрабатываю разные кода ошибок
		fprintf(stderr, "Error opening file: \n");
		result_code = get_error_code(result_code);
		goto cleanup;
	}

	// найти информацию о потоках
	result_code = avformat_find_stream_info(format_context, NULL);
	if (result_code < 0)
	{
		fprintf(stderr, "Error finding stream info: \n");
		result_code = get_error_code(result_code);
		goto cleanup;
	}

	// найти индекс поток
	int audio_stream_index = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (audio_stream_index < 0)
	{
		fprintf(stderr, "Audio stream not found\n");
		result_code = ERROR_UNSUPPORTED;
		goto cleanup;
	}

	// взять поток
	AVStream *audio_stream = format_context->streams[audio_stream_index];
	// найти декодер
	const AVCodec *audio_decoder = avcodec_find_decoder(audio_stream->codecpar->codec_id);
	if (!audio_decoder)
	{
		fprintf(stderr, "Could not find the decoder\n");
		result_code = ERROR_UNSUPPORTED;
		goto cleanup;
	}

	// создание нового контекста, связанного с выбранным декодером
	codec_context = avcodec_alloc_context3(audio_decoder);
	if (!codec_context)
	{
		fprintf(stderr, "Failed to allocate memory for the context codec\n");
		result_code = ERROR_NOTENOUGH_MEMORY;
		goto cleanup;
	}

	// передача параметров кодека из потока в контекст
	result_code = avcodec_parameters_to_context(codec_context, audio_stream->codecpar);
	if (result_code < 0)
	{
		fprintf(stderr, "Failed to copy codec parameters to context\n");
		result_code = get_error_code(result_code);
		goto cleanup;
	}

	// открыть кодек
	if (avcodec_open2(codec_context, audio_decoder, NULL) < 0)
	{
		fprintf(stderr, "Could not open codec\n");
		result_code = get_error_code(result_code);
		goto cleanup;
	}

	// проверка, что есть раскладка каналов
	if (!codec_context->channel_layout)
		// иначе установить по умолчанию
		codec_context->channel_layout = av_get_default_channel_layout(codec_context->channels);

	// создание и настройка контекста для ресемплинга
	resample_context = swr_alloc_set_opts(
		NULL,
		codec_context->channel_layout,
		// раскладку каналов какую хотим получить
		AV_SAMPLE_FMT_S16,
		// в какой формат
		codec_context->sample_rate,
		// частота которую хотим получить
		codec_context->channel_layout,
		// текущий расклад каналов
		codec_context->sample_fmt,
		// формат семплов который мы имеем сейчас
		codec_context->sample_rate,
		// текущая частота дискретизации
		0,
		NULL);

	// инициализация ресемплера
	if (!resample_context || swr_init(resample_context) < 0)
	{
		fprintf(stderr, "Failed to initialize the resampling context\n");
		result_code = ERROR_UNSUPPORTED;
		goto cleanup;
	}

	audio_data->sample_rate = codec_context->sample_rate;
	audio_data->channels = codec_context->channels;
	audio_data->sample_count = 0;
	audio_data->samples = NULL;

	// создание буфера для ресепмлинга
	int max_resampled_samples = 1024;
	if (av_samples_alloc_array_and_samples(&resampled_data, &resampled_data_size, codec_context->channels, max_resampled_samples, AV_SAMPLE_FMT_S16, 0) < 0)
	{
		fprintf(stderr, "Failed to allocate memory for resampled data\n");
		result_code = ERROR_NOTENOUGH_MEMORY;
		goto cleanup;
	}

	// вместимость сэмплов
	size_t samples_capacity = 16384;
	audio_data->samples = malloc(samples_capacity * sizeof(int16_t));
	if (!audio_data->samples)
	{
		fprintf(stderr, "Failed to allocate memory for the sample buffer\n");
		result_code = ERROR_NOTENOUGH_MEMORY;
		goto cleanup;
	}

	//* увеличение размера в 2 раза должно уменьшить количество повторного выделения и очистки данных
	while (av_read_frame(format_context, audio_packet) >= 0)
	{
		// отправка пакета на декодирование
		//* добавлена проверка
		if (avcodec_send_packet(codec_context, audio_packet) < 0)
		{
			fprintf(stderr, "Error sending packet for decoding\n");
			result_code = ERROR_UNSUPPORTED;
			goto cleanup;
		}

		// извлекает по фрейму пока может
		while (avcodec_receive_frame(codec_context, audio_frame) == 0)
		{
			int num_resampled_samples = audio_frame->nb_samples;
			// увеличить память если необходимо
			if (num_resampled_samples > max_resampled_samples)
			{
				if (resampled_data)
				{
					// очистить
					av_freep(&resampled_data[0]);
					av_freep(&resampled_data);
				}
				// выделить
				if (av_samples_alloc_array_and_samples(&resampled_data, &resampled_data_size, codec_context->channels, num_resampled_samples, AV_SAMPLE_FMT_S16, 0) <
					0)
				{
					fprintf(stderr, "Failed to allocate memory for resampled data\n");
					result_code = ERROR_NOTENOUGH_MEMORY;
					goto cleanup;
				}
				max_resampled_samples = num_resampled_samples;
			}

			// преобразование в нужный формат семплов
			int num_converted_samples = swr_convert(
				resample_context,
				resampled_data,
				num_resampled_samples,
				(const uint8_t **)audio_frame->data,
				audio_frame->nb_samples);

			if (num_converted_samples < 0)
			{
				fprintf(stderr, "Error converting samples\n");
				result_code = ERROR_UNSUPPORTED;
				goto cleanup;
			}

			// определить размер буфера
			int buffer_size =
				av_samples_get_buffer_size(NULL, codec_context->channels, num_converted_samples, AV_SAMPLE_FMT_S16, 0);

			if (buffer_size < 0)
			{
				fprintf(stderr, "Failed to determine buffer size for samples\n");
				result_code = ERROR_UNSUPPORTED;
				goto cleanup;
			}

			// если надо - выделить память
			size_t new_sample_count = audio_data->sample_count + num_converted_samples * codec_context->channels;
			if (new_sample_count > samples_capacity)
			{
				while (new_sample_count > samples_capacity)
				{
					samples_capacity *= 2;
				}
				int16_t *new_samples_buffer = realloc(audio_data->samples, samples_capacity * sizeof(int16_t));
				if (!new_samples_buffer)
				{
					fprintf(stderr, "Failed to reallocate memory for the sample buffer\n");
					result_code = ERROR_NOTENOUGH_MEMORY;
					goto cleanup;
				}
				audio_data->samples = new_samples_buffer;
			}
			// перенести куда, откуда, сколько ( с последнего сэпла и вперед )
			memcpy(audio_data->samples + audio_data->sample_count, resampled_data[0], buffer_size);
			audio_data->sample_count = new_sample_count;
		}
		av_packet_unref(audio_packet);
	}

	result_code = SUCCESS;

cleanup:
	if (resampled_data)
	{
		av_freep(&resampled_data[0]);
		av_freep(&resampled_data);
	}
	CLEANUP(resample_context, swr_free);
	CLEANUP(audio_frame, av_frame_free);
	CLEANUP(audio_packet, av_packet_free);
	CLEANUP(codec_context, avcodec_free_context);
	CLEANUP(format_context, avformat_close_input);
	return result_code;
}

void delete_audio_data(AudioData *audio_data)
{
	if (audio_data->samples)
	{
		free(audio_data->samples);
		audio_data->samples = NULL;
	}
	audio_data->sample_count = 0;
	audio_data->sample_rate = 0;
	audio_data->channels = 0;
}

#endif
