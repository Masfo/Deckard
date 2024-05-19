module;

export module deckard.filemonitor;
import std;

struct MonitorHash
{
	using is_transparent = void;

	std::size_t operator()(const std::string& r) const { return std::hash<std::string>{}(r); }
};

export namespace deckard
{
	export enum class StatusFlag {
		Created,
		Modified,
		Deleted,
	};

	export enum class FileType {
		File,
		Folder,
	};

	export struct [[nodiscard("Path data")]] alignas(64) MonitorData
	{

		std::filesystem::path           path;
		std::filesystem::file_time_type time;
		StatusFlag                      statuscode{StatusFlag::Created};
		bool                            directory{false};

		bool is_directory() const { return directory; }

		size_t filesize() const { return std::filesystem::file_size(path); }

		std::string filename() const { return path.filename().string(); }

		std::string filepath() const { return path.string(); };

		bool empty() const { return path.empty(); };

		bool operator==(const MonitorData& rhs) const { return path == rhs.path; }

		std::string_view typestring() const
		{
			if (is_directory())
				return "directory";
			else
				return "file";
		}

		FileType type() const
		{
			if (is_directory())
				return FileType::Folder;
			else
				return FileType::File;
		}

		std::string_view statusstring() const
		{
			if (statuscode == StatusFlag::Created)
				return "created";
			else if (statuscode == StatusFlag::Modified)
				return "modified";
			else if (statuscode == StatusFlag::Deleted)
				return "deleted";
			return "";
		}

		StatusFlag status() const { return statuscode; }
	};

	export class Monitor
	{
		using UserFunction = void(const MonitorData&);

	public:
		explicit Monitor(const std::filesystem::path& path) noexcept
			: m_current_path(path)
		{
		}

		Monitor() noexcept
			: Monitor(std::filesystem::current_path())
		{
		}

		bool filter(std::string_view) const noexcept { return false; }

		// Copy
		Monitor(Monitor const&)            = delete;
		Monitor& operator=(Monitor const&) = delete;
		// Move
		Monitor(Monitor&&)            = delete;
		Monitor& operator=(Monitor&&) = delete;

		~Monitor() noexcept = default;

		template<typename Callable>
		void start(Callable func, const std::chrono::milliseconds update_time = std::chrono::milliseconds{1'000}) noexcept
		{
			static_assert(std::is_convertible_v<Callable&&, std::function<UserFunction>>, "Wrong callback signature");
			m_callback = func;

			start_thread(update_time);
		}

	private:
		void start_thread(std::chrono::milliseconds update_time = std::chrono::milliseconds{1'000}) noexcept
		{

			m_monitor_thread = std::jthread(
			  [&, update_time](std::stop_token token)
			  {
				  // First scan
				  for (const auto& file : std::filesystem::recursive_directory_iterator(
						 m_current_path, std::filesystem::directory_options::skip_permission_denied))
					  update_file(file, StatusFlag::Created);

				  while (!token.stop_requested())
				  {
					  std::this_thread::sleep_for(update_time);
					  update_files();
				  }
			  });
		}

		void callback(const MonitorData& data) const noexcept
		{
			//
			m_callback(data);
		}

		bool contains(const std::string& sv) const noexcept { return m_files.contains(sv); }

		void update_file(const std::filesystem::directory_entry& path, StatusFlag flag) noexcept
		{
			const auto& str         = path.path().string();
			m_files[str].path       = path;
			m_files[str].directory  = std::filesystem::is_directory(path);
			m_files[str].statuscode = flag;
			m_files[str].time       = std::filesystem::last_write_time(path);
		}

		void update_deleted_files() noexcept
		{
			// Delete files
			std::erase_if(m_files,
						  [&](auto& it)
						  {
							  if (!std::filesystem::exists(it.first))
							  {
								  it.second.statuscode = StatusFlag::Deleted;
								  callback(it.second);
								  return true;
							  }
							  return false;
						  });
		}

		void update_files() noexcept
		{
			for (const auto& file :
				 std::filesystem::recursive_directory_iterator(m_current_path, std::filesystem::directory_options::skip_permission_denied))
			{
				const auto& filestring = file.path().string();

				if (!contains(filestring))
				{
					// Add new files/folders
					update_file(file, StatusFlag::Created);
					callback(m_files[filestring]);
				}
				else
				{
					if (m_files[filestring].time != std::filesystem::last_write_time(file))
					{

						// Detect file/folder modification
						update_file(file, StatusFlag::Modified);
						callback(m_files[filestring]);
					}
				}
			}

			update_deleted_files();
		}

		std::unordered_map<std::string, MonitorData, MonitorHash, std::equal_to<>> m_files;
		std::filesystem::path                                                      m_current_path;
		std::jthread                                                               m_monitor_thread;
		UserFunction*                                                              m_callback{nullptr};
		unsigned int                                                               m_filter{0};
	};

} // namespace deckard
