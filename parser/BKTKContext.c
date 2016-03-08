#include "BKWaveFileReader.h"
#include "BKTKContext.h"
#include "BKTKInterpreter.h"

extern BKClass const BKTKContextClass;
extern BKClass const BKTKGroupClass;
extern BKClass const BKTKTrackClass;
extern BKClass const BKTKInstrumentClass;
extern BKClass const BKTKWaveformClass;
extern BKClass const BKTKSampleClass;

static void printError (BKTKContext * ctx, char const * format, ...)
{
	va_list args;

	va_start (args, format);
	BKStringAppendFormatArgs (&ctx -> error, format, args);
	BKStringAppend (&ctx -> error, "\n");
	va_end (args);
}

BKInt BKTKTrackAlloc (BKTKTrack ** track)
{
	BKInt res;

	if ((res = BKObjectInit (track, &BKTKTrackClass, sizeof (*track))) != 0) {
		return res;
	}

	(*track) -> byteCode = BK_BYTE_BUFFER_INIT;
	(*track) -> groups = BK_ARRAY_INIT (sizeof (BKTKGroup *));

	return res;
}

BKInt BKTKGroupAlloc (BKTKGroup ** group)
{
	BKInt res;

	if ((res = BKObjectAlloc ((void **) group, &BKTKGroupClass, sizeof (*group))) != 0) {
		return res;
	}

	(*group) -> byteCode = BK_BYTE_BUFFER_INIT;

	return res;
}

BKInt BKTKInstrumentAlloc (BKTKInstrument ** instrument)
{
	BKInt res;

	if ((res = BKObjectAlloc ((void **) instrument, &BKTKInstrumentClass, sizeof (*instrument))) != 0) {
		return res;
	}

	if ((res = BKInstrumentInit (&(*instrument) -> instr)) != 0) {
		return res;
	}

	return res;
}

BKInt BKTKWaveformAlloc (BKTKWaveform ** waveform)
{
	BKInt res;

	if ((res = BKObjectAlloc ((void **) waveform, &BKTKWaveformClass, sizeof (*waveform))) != 0) {
		return res;
	}

	if ((res = BKDataInit (&(*waveform) -> data)) != 0) {
		return res;
	}

	return res;
}

BKInt BKTKSampleAlloc (BKTKSample ** sample)
{
	BKInt res;

	if ((res = BKObjectAlloc ((void **) sample, &BKTKSampleClass, sizeof (*sample))) != 0) {
		return res;
	}

	if ((res = BKDataInit (&(*sample) -> data)) != 0) {
		return res;
	}

	return res;
}

static void BKTKGroupDispose (BKTKGroup * group)
{
	if (group) {
		BKByteBufferDispose (&group -> byteCode);
		free (group);
	}
}

static void BKTKTrackDispose (BKTKTrack * track)
{
	BKTKGroup * group;

	if (track) {
		for	(size_t i = 0; i < track -> groups.len; i ++) {
			group = *(BKTKGroup **) BKArrayItemAt (&track -> groups, i);
			BKTKGroupDispose (group);
		}

		BKByteBufferDispose (&track -> byteCode);
		BKDispose (&track -> renderTrack);
		BKDividerDetach (&track -> divider);
		BKDispose (&track -> interpreter);
		free (track);
	}
}

static void BKTKInstrumentDispose (BKTKInstrument * instrument)
{
	if (instrument) {
		BKDispose (&instrument -> instr);
		free (instrument);
	}
}

static void BKTKWaveformDispose (BKTKWaveform * waveform)
{
	if (waveform) {
		BKDispose (&waveform -> data);
		free (waveform);
	}
}

static void BKTKSampleDispose (BKTKSample * sample)
{
	if (sample) {
		BKStringDispose (&sample -> path);
		BKDispose (&sample -> data);
		free (sample);
	}
}

BKInt BKTKContextInit (BKTKContext * ctx)
{
	BKInt res;

	if ((res = BKObjectInit (ctx, &BKTKContextClass, sizeof (*ctx))) != 0) {
		return res;
	}

	ctx -> instruments = BK_ARRAY_INIT (sizeof (BKTKInstrument *));
	ctx -> waveforms = BK_ARRAY_INIT (sizeof (BKTKWaveform *));
	ctx -> samples = BK_ARRAY_INIT (sizeof (BKTKSample *));
	ctx -> tracks = BK_ARRAY_INIT (sizeof (BKTKTrack *));
	ctx -> error = BK_STRING_INIT;
	ctx -> loadPath = BK_STRING_INIT;

	return 0;
}

static BKInt BKTKContextLoadSamples (BKTKContext * ctx, BKTKCompiler * compiler)
{
	BKInt res = 0;
	FILE * file = NULL;
	BKInt numFrames, numChannels, sampleRate;
	BKFrame * frames = NULL;
	BKTKSample * sample;
	BKWaveFileReader reader;
	BKHashTableIterator itor;
	char const * key;
	BKString dir = BK_STRING_INIT;
	BKString path = BK_STRING_INIT;
	BKArray * samples = &ctx -> samples;

	BKStringAppendString (&dir, &ctx -> loadPath);

	BKHashTableIteratorInit (&itor, &compiler -> samples);

	if (BKArrayResize (samples, BKHashTableSize (&compiler -> samples)) != 0) {
		printError (ctx, "Error: allocation error");
		goto allocationError;
	}

	while (BKHashTableIteratorNext (&itor, &key, (void **) &sample)) {
		*(BKTKSample **) BKArrayItemAt (samples, sample -> object.index) = sample;

		if (sample -> path.len) {
			BKStringEmpty (&path);
			BKStringAppendString (&path, &dir);
			BKStringAppendPathSegment (&path, &sample -> path);

			file = fopen ((char *) path.str, "rb");

			if (!file) {
				printError (ctx, "Error: opening file failed: '%s' on line %u:%u",
					sample -> path.str, sample -> object.offset.lineno, sample -> object.offset.colno);
				res = BK_FILE_ERROR;
				goto cleanup;
			}

			if (BKWaveFileReaderInit (&reader, file) != 0) {
				printError (ctx, "Error: allocation error");
				goto cleanup;
			}

			if (BKWaveFileReaderReadHeader (&reader, &numChannels, &sampleRate, &numFrames) != 0) {
				printError (ctx, "Error: allocation error");
				goto allocationError;
			}

			frames = malloc (numFrames * numChannels * sizeof (BKFrame));

			if (!frames) {
				printError (ctx, "Error: allocation error");
				goto allocationError;
			}

			if (BKWaveFileReaderReadFrames (&reader, frames) != 0) {
				printError (ctx, "Error: allocation error");
				goto allocationError;
			}

			if (BKDataSetFrames (&sample -> data, frames, numFrames, numChannels, 1)) {
				printError (ctx, "Error: allocation error");
				goto allocationError;
			}

			BKDispose (&reader);

			free (frames);
			frames = NULL;

			fclose (file);
			file = NULL;
		}

		if (sample -> sustainRange [0] || sample -> sustainRange [1]) {
			BKSetPtr (&sample -> data, BK_SAMPLE_SUSTAIN_RANGE, &sample -> sustainRange, sizeof (sample -> sustainRange));
		}

		BKSetAttr (&sample -> data, BK_SAMPLE_PITCH, (BKInt) (((uint64_t) sample -> pitch * BK_FINT20_UNIT) / 100));
	}

	cleanup: {
		if (file) {
			fclose (file);
		}

		if (frames) {
			free (frames);
		}

		BKStringDispose (&dir);
		BKStringDispose (&path);

		return res;
	}

	allocationError: {
		res = BK_ALLOCATION_ERROR;
		goto cleanup;
	}
}

static BKInt BKTKContextCreateTracks (BKTKContext * ctx, BKTKCompiler * compiler)
{
	BKInt res = 0;
	BKTKTrack * track;
	BKTKTrack ** trackRef;

	if (BKArrayResize (&ctx -> tracks, compiler -> tracks.len)) {
		printError (ctx, "Error: allocation error");
		goto allocationError;
	}

	for (size_t i = 0; i < compiler -> tracks.len; i ++) {
		trackRef = BKArrayItemAt (&compiler -> tracks, i);
		track = *trackRef;

		if (!track) {
			continue;
		}

		if ((res = BKTKInterpreterInit (&track -> interpreter)) != 0) {
			goto cleanup;
		}

		if ((res = BKTrackInit (&track -> renderTrack, BK_SQUARE)) != 0) {
			goto cleanup;
		}

		BKSetAttr (&track -> renderTrack, BK_VOLUME, BK_MAX_VOLUME);

		track -> interpreter.opcode = track -> byteCode.first -> data;
		track -> interpreter.opcodePtr = track -> interpreter.opcode;
		track -> ctx = ctx;

		*(BKTKTrack **) BKArrayItemAt (&ctx -> tracks, i) = track;
	}

	cleanup: {
		return res;
	}

	allocationError: {
		res = BK_ALLOCATION_ERROR;
		goto cleanup;
	}
}

BKInt BKTKContextCreate (BKTKContext * ctx, BKTKCompiler * compiler)
{
	BKInt res = 0;
	BKTKWaveform * waveform;
	BKTKInstrument * instrument;
	BKArray * waveforms = &ctx -> waveforms;
	BKArray * instruments = &ctx -> instruments;
	BKHashTableIterator itor;
	char const * key;

	BKHashTableIteratorInit (&itor, &compiler -> instruments);

	if (BKArrayResize (instruments, BKHashTableSize (&compiler -> instruments)) != 0) {
		printError (ctx, "Error: allocation error");
		goto allocationError;
	}

	while (BKHashTableIteratorNext (&itor, &key, (void **) &instrument)) {
		*(BKTKInstrument **) BKArrayItemAt (instruments, instrument -> object.index) = instrument;
	}

	BKHashTableIteratorInit (&itor, &compiler -> waveforms);

	if (BKArrayResize (waveforms, BKHashTableSize (&compiler -> waveforms)) != 0) {
		printError (ctx, "Error: allocation error");
		goto allocationError;
	}

	while (BKHashTableIteratorNext (&itor, &key, (void **) &waveform)) {
		*(BKTKWaveform **) BKArrayItemAt (waveforms, waveform -> object.index) = waveform;
	}

	if ((res = BKTKContextLoadSamples (ctx, compiler)) != 0) {
		goto cleanup;
	}

	if ((res = BKTKContextCreateTracks (ctx, compiler)) != 0) {
		goto cleanup;
	}

	BKArrayEmpty (&compiler -> tracks);
	BKHashTableEmpty (&compiler -> instruments);
	BKHashTableEmpty (&compiler -> waveforms);
	BKHashTableEmpty (&compiler -> samples);

	BKTKCompilerReset (compiler);

	cleanup: {
		return res;
	}

	allocationError: {
		res = BK_ALLOCATION_ERROR;
		goto cleanup;
	}
}

static BKEnum dividerCallback (BKCallbackInfo * info, BKTKTrack * track)
{
	BKInt ticks;

	BKTKInterpreterAdvance (&track -> interpreter, track, &ticks);
	info -> divider = ticks;

	return 0;
}

BKInt BKTKContextAttach (BKTKContext * ctx, BKContext * renderContext)
{
	BKInt res;
	BKTKTrack * track;
	BKCallback callback;

	if (ctx -> renderContext) {
		return BK_INVALID_STATE;
	}

	ctx -> renderContext = renderContext;
	callback.func = (BKCallbackFunc) dividerCallback;

	for (size_t i = 0; i < ctx -> tracks.len; i ++) {
		track = *(BKTKTrack **) BKArrayItemAt (&ctx -> tracks, i);

		if (track) {
			if ((res = BKTrackAttach (&track -> renderTrack, ctx -> renderContext)) != 0) {
				return res;
			}

			callback.userInfo = track;

			if ((res = BKDividerInit (&track -> divider, 0, &callback)) != 0) {
				return res;
			}

			if ((res = BKContextAttachDivider (ctx -> renderContext, &track -> divider, BK_CLOCK_TYPE_BEAT)) != 0) {
				return res;
			}
		}
	}

	return 0;
}

void BKTKContextDetach (BKTKContext * ctx)
{
	BKTKTrack * track;

	if (!ctx -> renderContext) {
		return;
	}

	for (size_t i = 0; i < ctx -> tracks.len; i ++) {
		track = *(BKTKTrack **) BKArrayItemAt (&ctx -> tracks, i);

		if (track) {
			BKDividerDetach (&track -> divider);
			BKTrackDetach (&track -> renderTrack);
		}
	}

	ctx -> renderContext = NULL;
}

void BKTKContextReset (BKTKContext * ctx)
{
	BKTKTrack * track;

	for (size_t i = 0; i < ctx -> tracks.len; i ++) {
		track = *(BKTKTrack **) BKArrayItemAt (&ctx -> tracks, i);

		BKDividerReset (&track -> divider);
		BKTKInterpreterReset (&track -> interpreter);
		BKTrackReset (&track -> renderTrack);
	}

	BKStringEmpty (&ctx -> error);
}

static void BKTKContextDispose (BKTKContext * ctx)
{
	for (size_t i = 0; i < ctx -> instruments.len; i ++) {
		BKTKInstrumentDispose (*(BKTKInstrument **) BKArrayItemAt (&ctx -> instruments, i));
	}

	for (size_t i = 0; i < ctx -> waveforms.len; i ++) {
		BKTKWaveformDispose (*(BKTKWaveform **)BKArrayItemAt (&ctx -> waveforms, i));
	}

	for (size_t i = 0; i < ctx -> samples.len; i ++) {
		BKTKSampleDispose (*(BKTKSample **)BKArrayItemAt (&ctx -> samples, i));
	}

	for (size_t i = 0; i < ctx -> tracks.len; i ++) {
		BKTKTrackDispose (*(BKTKTrack **)BKArrayItemAt (&ctx -> tracks, i));
	}

	BKArrayDispose (&ctx -> instruments);
	BKArrayDispose (&ctx -> waveforms);
	BKArrayDispose (&ctx -> samples);
	BKArrayDispose (&ctx -> tracks);
}

BKClass const BKTKContextClass =
{
	.instanceSize = sizeof (BKTKContext),
	.dispose      = (void *) BKTKContextDispose,
};

BKClass const BKTKGroupClass =
{
	.instanceSize = sizeof (BKTKGroup),
	.dispose      = (void *) BKTKGroupDispose,
};

BKClass const BKTKTrackClass =
{
	.instanceSize = sizeof (BKTKTrack),
	.dispose      = (void *) BKTKTrackDispose,
};

BKClass const BKTKInstrumentClass =
{
	.instanceSize = sizeof (BKTKInstrument),
	.dispose      = (void *) BKTKInstrumentDispose,
};

BKClass const BKTKWaveformClass =
{
	.instanceSize = sizeof (BKTKWaveform),
	.dispose      = (void *) BKTKWaveformDispose,
};

BKClass const BKTKSampleClass =
{
	.instanceSize = sizeof (BKTKSample),
	.dispose      = (void *) BKTKSampleDispose,
};