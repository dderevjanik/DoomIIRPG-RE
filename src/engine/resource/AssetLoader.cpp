#include "AssetLoader.h"

#include <format>

#include "CAppContainer.h"
#include "Log.h"
#include "VFS.h"

AssetLoader::AssetLoader() = default;

AssetLoader::~AssetLoader() {
	this->shutdown();
}

void AssetLoader::start() {
	if (this->running.exchange(true)) return; // already running
	this->quit = false;
	this->worker = std::thread([this]() { this->workerLoop(); });
}

void AssetLoader::shutdown() {
	if (!this->running.exchange(false)) return; // not running
	{
		std::lock_guard<std::mutex> lk(this->jobMutex);
		this->quit = true;
	}
	this->jobCv.notify_all();
	if (this->worker.joinable()) {
		this->worker.join();
	}
}

void AssetLoader::enqueueSoundLoad(int resID, std::string fileName) {
	{
		std::lock_guard<std::mutex> lk(this->jobMutex);
		this->jobs.push_back(Job{resID, std::move(fileName)});
	}
	this->jobCv.notify_one();
}

bool AssetLoader::tryPopResult(Result& out) {
	std::lock_guard<std::mutex> lk(this->resultMutex);
	if (this->results.empty()) return false;
	out = std::move(this->results.front());
	this->results.pop_front();
	return true;
}

void AssetLoader::workerLoop() {
	VFS* vfs = CAppContainer::getInstance()->vfs;

	while (true) {
		Job job;
		{
			std::unique_lock<std::mutex> lk(this->jobMutex);
			this->jobCv.wait(lk, [this]() { return this->quit || !this->jobs.empty(); });
			if (this->quit && this->jobs.empty()) return;
			job = std::move(this->jobs.front());
			this->jobs.pop_front();
		}

		Result r;
		r.resID = job.resID;
		auto path = std::format("audio/{}", job.fileName);
		int sizeOut = 0;
		uint8_t* data = vfs->readFile(path.c_str(), &sizeOut);
		if (data && sizeOut > 0) {
			r.bytes.assign(data, data + sizeOut);
			r.success = true;
			std::free(data); // VFS::readFile returns malloc'd buffer
		} else {
			r.success = false;
			LOG_WARN("[asset-loader] failed to read {}\n", path);
		}

		{
			std::lock_guard<std::mutex> lk(this->resultMutex);
			this->results.push_back(std::move(r));
		}
	}
}
