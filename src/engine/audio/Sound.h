#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include <al.h>
#include <alc.h>
#include "AssetLoader.h"

class InputStream;

typedef uint32_t AudioFormatID;
typedef uint32_t AudioFormatFlags;
typedef uint32_t AudioFileID;
struct AudioStreamBasicDescription
{
	uint32_t mSampleRate;
	AudioFormatID mFormatID;
	AudioFormatFlags mFormatFlags;
	uint32_t mBytesPerPacket;
	uint32_t mFramesPerPacket;
	uint32_t mBytesPerFrame;
	uint32_t mChannelsPerFrame;
	uint32_t mBitsPerChannel;
	uint32_t mReserved;
};

class Applet;


class Sound
{
private:

public:

	// Streamed music playback — keeps a file handle open and feeds OpenAL via
	// 4 small queued buffers refilled per-frame. Avoids the 10 MB-per-track RAM
	// cost and the multi-tens-of-ms first-play stall of full alBufferData loads.
	// Auto-detects WAV (RIFF) and Ogg Vorbis (OggS) by file magic — Ogg support
	// is gated by DRPG_HAS_VORBIS.
	class MusicStream {
	public:
		static constexpr int NUM_BUFFERS = 4;
		static constexpr int CHUNK_BYTES = 16 * 1024;

		std::unique_ptr<InputStream> stream;
		ALuint streamBuffers[NUM_BUFFERS] = {};
		ALenum format = 0;
		int sampleRate = 0;
		// WAV path uses dataOffset+dataSize+readCursor as a byte cursor into
		// the file. Ogg path leaves them zero and uses the OggDecoder pimpl.
		int dataOffset = 0;
		int dataSize = 0;
		int readCursor = 0;
		bool looping = false;
		bool eof = false;
		bool isOgg = false;
		// Pimpl, only allocated when isOgg. Forward-declared here so callers
		// don't need to pull in <vorbis/vorbisfile.h>.
		std::unique_ptr<struct OggDecoder> ogg;

		MusicStream();
		~MusicStream();

		// Open the music file at fileName (WAV or Ogg), allocate AL buffers,
		// pre-fill them, and start playing on `source`. Returns false on any
		// failure (caller falls back to non-streaming load).
		bool start(const char* fileName, ALuint source, bool loop);
		// Per-frame refill: unqueue any drained buffers and refill from the file.
		void update(ALuint source);
		// Stop playback, drain queued buffers, free AL buffers + close file.
		void stop(ALuint source);

	private:
		// Read up to CHUNK_BYTES bytes into `dst`. Dispatches to the WAV or Ogg
		// decoder based on isOgg. Returns bytes actually read (0 on hard EOF
		// when !looping).
		int readNextChunk(uint8_t* dst);
		int readNextWavChunk(uint8_t* dst);
		int readNextOggChunk(uint8_t* dst);
	};

	class SoundStream
	{
	public:
		ALuint bufferId = 0;
		ALuint sourceId = 0;
		int16_t resID = 0;
		int16_t priority = 0;
		bool fadeInProgress = false;
		uint8_t pad_0xd = 0;
		uint8_t pad_0xe = 0;
		uint8_t pad_0xf = 0;
		int volume = 0;
		int fadeBeg = 0;
		int fadetime = 0;
		float fadeVolume = 0.0f;
		// Non-null only for music tracks (resID 1067..1071). When set, the slot
		// is in streaming mode: alBufferData was NOT called; instead the stream
		// queues alSourceQueueBuffers chunks, refilled from Sound::startFrame().
		std::unique_ptr<MusicStream> music;

		void StartFade(int volume, int fadeBeg, int fadeEnd);
	};

	Applet* app = nullptr;
	bool headless = false;
	bool isMuted = false; // guessed — blocks sound playback when true
	bool allowSounds = false;
	bool allowMusics = false;
	int soundFxVolume = 0;
	int musicVolume = 0;
	int soundCapacity = 0; // guessed — playback blocked when 0, initialized to 100
	static constexpr size_t MAX_CHANNELS = 64; // soft cap to bound OpenAL source pressure
	std::vector<SoundStream> channel;
	short resID = 0;
	uint8_t flags = 0;
	int priority = 0;
	int unused_0x15c = 0; // set to -1 but never read
	short unused_0x160 = 0; // set to -1 but never read
	uint8_t hasDeferredSound = 0; // guessed — triggers deferred playback in startFrame
	bool soundsLoaded = false;
	ALCcontext* alContext = nullptr;
	ALCdevice* alDevice = nullptr;

	// Background loader for SFX preloading. Worker thread reads WAV bytes off
	// the main thread; the main thread drains finished results in startFrame()
	// and publishes them to OpenAL via alBufferData, caching the resulting
	// buffer ids in preloadedBuffers so subsequent playSound calls skip the
	// per-play disk I/O entirely.
	AssetLoader assetLoader;
	std::unordered_map<int, ALuint> preloadedBuffers;

	// Constructor
	Sound();
	// Destructor
	~Sound();

	bool startup();
	void openAL_Init();
	void openAL_Close();
	void openAL_SetSystemVolume(int volume);
	void openAL_SetVolume(ALuint source, int volume);
	void openAL_Suspend();
	void openAL_Resume();
	bool openAL_IsPlaying(ALuint source);
	bool openAL_IsPaused(ALuint source);
	bool openAL_GetALFormat(AudioStreamBasicDescription aStreamBD, ALenum* format);
	void openAL_PlaySound(ALuint source, ALint loop);
	void openAL_LoadSound(int resID, Sound::SoundStream* channel);
	bool openAL_LoadWAVFromFile(ALuint bufferId, const char* fileName);
	bool openAL_LoadAudioFileData(const char* fileName, ALenum* format, std::vector<uint8_t>& data, ALsizei* freq);
	bool openAL_OpenAudioFile(const char* fileName, InputStream* IS);
	bool openAL_LoadAllSounds();

	bool cacheSounds();
	// Kick off async pre-loading of every non-music SFX in the catalog. Idempotent.
	void preloadAllSFX();
	// Drain finished AssetLoader results, publish to OpenAL, populate preloadedBuffers.
	void drainPreloadResults();
	void playSound(int16_t resID, uint8_t flags, int priority, bool unused); // guessed — param never read
	int getFreeSlot(int minPriority); // guessed
	int allocateChannel();
	void soundStop();
	void stopSound(int resID, bool fadeOut);
	bool isSoundPlaying(int16_t resID);
	void updateVolume();
	void playCombatSound(int16_t resID, uint8_t flags, int priority);
	bool cacheCombatSound(int resID);
	void freeMonsterSounds();

	void volumeUp(int volume);
	void volumeDown(int volume);
	void startFrame();
	void endFrame();
	void musicVolumeUp(int volume); // [GEC]
	void musicVolumeDown(int volume); // [GEC]
};
