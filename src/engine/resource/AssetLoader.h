#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Background asset I/O worker. Reads files (currently: WAV bytes for SFX) on a
// dedicated thread so the main thread is never blocked on disk I/O during a
// loading screen or the first play of an unfamiliar sound.
//
// The worker performs disk I/O only — no OpenAL or other engine state is
// touched. Producers enqueue jobs by `(resID, fileName, loadType)`; the main
// thread later drains finished `Result`s and is responsible for publishing
// them to whatever subsystem cares (Sound calls alBufferData on the result
// bytes).
class AssetLoader {
public:
	struct Result {
		int resID = -1;
		std::vector<uint8_t> bytes;
		bool success = false;
	};

	AssetLoader();
	~AssetLoader();

	// Spawn the worker thread. Idempotent.
	void start();

	// Stop accepting jobs, join the worker. Idempotent.
	void shutdown();

	// Enqueue a sound-resource load. The worker will read the file via VFS and
	// post a Result back. `fileName` is the bare audio filename (e.g.
	// "pistol.wav") — the worker prefixes "audio/" the same way Sound does.
	void enqueueSoundLoad(int resID, std::string fileName);

	// Drain a single finished result. Returns false if the queue is empty.
	bool tryPopResult(Result& out);

private:
	struct Job {
		int resID;
		std::string fileName;
	};

	void workerLoop();

	std::thread worker;
	std::atomic<bool> running{false};
	std::atomic<bool> quit{false};

	std::mutex jobMutex;
	std::condition_variable jobCv;
	std::deque<Job> jobs;

	std::mutex resultMutex;
	std::deque<Result> results;
};
