///
/// utility: suprocess
///
/// Inspired by Tero Saarni (see https://github.com/tsaarni/cpp-subprocess)
///
/// License: MIT
///
/// Copyright (c) 2016 Mario Konrad
///

#ifndef __UTILS__SUBPROCESS__HPP__
#define __UTILS__SUBPROCESS__HPP__

#include <string>
#include <system_error>
#include <vector>
#include <istream>
#include <ostream>
#include <memory>

#include <unistd.h>
#include <sys/wait.h>

#include <ext/stdio_filebuf.h>

namespace utils
{
///
/// \brief Executes commands in a subprocess while providing streams
///        to communicate with the child process.
///
/// \note This implementation is GCC specific.
///
/// \note This class is not thread safe.
///
/// Example:
/// \code
///   utils::subprocess p{{"ls", "-l", "-a"}};
///
///   p.exec();
///
///   p.out() >> std::noskipws;
///   std::copy(
///       std::istream_iterator<char>{p.out()},
///       std::istream_iterator<char>{},
///       std::ostream_iterator<char>{std::cout});
///
///   p.wait();
/// \endcode
///
/// This implementation is restartable, example:
/// \code
///   utils::subprocess p{{"ls", "-l", "-a"}};
///
///   p.exec();
///   p.wait();
///
///   p.exec();
///   p.wait();
/// \endcode
///
/// **Caution:** it is not possible to parallize the execution,
/// example:
/// \code
///   utils::subprocess p{{"ls", "-l", "-a"}};
///
///   p.exec();
///   p.exec(); // error, exception is thrown
///   p.wait();
/// \endcode
///
class subprocess final
{
public:
	/// Initializes the object with a container of string, representing
	/// the command to execute and its parameters.
	///
	/// \param[in] args Arguments to execute.
	///
	/// \note The construction is cheap and the object (once it is finished
	///       running), is reusable.
	subprocess(const std::vector<std::string> & args)
		: pid(-1)
		, args(args)
		, detached(false)
	{
	}

	/// Closes all pipes. If not detached (default), it also waits for
	/// the subprocess to terminate.
	~subprocess()
	{
		if (!detached) {
			wait();
		}
		close_all();
	}

	/// Detaches the subprocess in the sense of not waiting for termination
	/// upon destruction of subprocess object. However, the pipes will be closed
	/// on destruction of the subprocess object.
	void detach() { detached = true; }

	std::ostream & in() { return *stream_in; }
	std::istream & out() { return *stream_out; }
	std::istream & err() { return *stream_err; }

	/// Closes the input stream, signalling the subprocess an end of file.
	void close_in()
	{
		if (filebuf_in) {
			filebuf_in->close();

			filebuf_in.reset();
			stream_in.reset();
		}
	}

	/// Waits for the child process to terminate and returns its exit code.
	///
	/// \return The exit code of the child process.
	int wait()
	{
		int status = 0;
		::waitpid(pid, &status, 0);
		pid = -1;
		return WEXITSTATUS(status);
	}

	/// Executes the child process.
	///
	/// \return This subprocess object.
	subprocess & operator()() { return exec(); }

	/// Executes the child process.
	///
	/// \return This subprocess object.
	subprocess & operator()(subprocess & destination) { return exec(destination); }

	/// Executes this subprocess and uses the specified subprocess.
	/// The output (stdout) of the subprocess will be used as the input
	/// (stdin) of the specified subprocess.
	///
	/// \note The specified subprocess is expected to be running already.
	///
	/// \note If the subprocess is already running, an exception is thrown.
	///
	/// \return This subprocess object.
	subprocess & exec(subprocess & destination)
	{
		ensure_not_running();

		// clean up if necessary then execute the subprocess
		close_all();
		reset_all_streams();

		// execution
		setup_pipes(&destination.in());
		execute();
		return *this;
	}

	/// Executes the child process.
	///
	/// \note If the subprocess is already running, an exception is thrown.
	///
	/// \return This subprocess object.
	subprocess & exec()
	{
		ensure_not_running();

		// clean up if necessary then execute the subprocess
		close_all();
		reset_all_streams();

		// execution
		setup_pipes();
		execute();
		return *this;
	}

private:
	using file_buffer = __gnu_cxx::stdio_filebuf<char>;

	enum pipe_end_type { READ = 0, WRITE = 1 };

	int pipe_in[2] = {-1, -1};
	int pipe_out[2] = {-1, -1};
	int pipe_err[2] = {-1, -1};

	pid_t pid;
	std::vector<std::string> args;
	bool detached;

	std::unique_ptr<file_buffer> filebuf_in;
	std::unique_ptr<file_buffer> filebuf_out;
	std::unique_ptr<file_buffer> filebuf_err;

	std::unique_ptr<std::ostream> stream_in;
	std::unique_ptr<std::istream> stream_out;
	std::unique_ptr<std::istream> stream_err;

	/// Closes a file descriptor and sets it to `-1` after closing.
	static void close_pipe(int & p)
	{
		if (p >= 0) {
			::close(p);
			p = -1;
		}
	}

	void ensure_not_running() const
	{
		if (pid >= 0)
			throw std::runtime_error{"repeated start"};
	}

	void setup_pipes(std::ostream * destination = nullptr)
	{
		// in
		if (::pipe(pipe_in) == -1)
			throw std::system_error(errno, std::system_category());

		// out
		if (destination) {
			pipe_out[WRITE] = dynamic_cast<file_buffer *>(destination->rdbuf())->fd();
		} else {
			if (::pipe(pipe_out) == -1)
				throw std::system_error(errno, std::system_category());
		}

		// err
		if (::pipe(pipe_err) == -1)
			throw std::system_error(errno, std::system_category());
	}

	/// Closes all pipes and marks them invalid.
	void close_all()
	{
		close_pipe(pipe_in[READ]);
		close_pipe(pipe_in[WRITE]);
		close_pipe(pipe_out[READ]);
		close_pipe(pipe_out[WRITE]);
		close_pipe(pipe_err[READ]);
		close_pipe(pipe_err[WRITE]);
	}

	/// Destorys all streams and filebuffer objects.
	void reset_all_streams()
	{
		filebuf_in.reset();
		filebuf_out.reset();
		filebuf_err.reset();

		stream_in.reset();
		stream_out.reset();
		stream_err.reset();
	}

	/// Start child process and execute stuff.
	void execute()
	{
		pid = ::fork();
		if (pid == -1)
			throw std::system_error(errno, std::system_category());

		if (pid == 0) {
			exec_child(); // does not return
		} else {
			exec_parent();
		}
	}

	void exec_parent()
	{
		close_pipe(pipe_in[READ]);
		close_pipe(pipe_out[WRITE]);
		close_pipe(pipe_err[WRITE]);

		filebuf_in = std::make_unique<file_buffer>(pipe_in[WRITE], std::ios_base::out, 1);
		stream_in = std::make_unique<std::ostream>(filebuf_in.get());

		filebuf_out = std::make_unique<file_buffer>(pipe_out[READ], std::ios_base::in, 1);
		stream_out = std::make_unique<std::istream>(filebuf_out.get());

		filebuf_err = std::make_unique<file_buffer>(pipe_err[READ], std::ios_base::in, 1);
		stream_err = std::make_unique<std::istream>(filebuf_err.get());
	}

	void exec_child()
	{
		struct mutable_string {
			mutable_string(std::string s)
				: data(s.c_str(), s.c_str() + s.size() + 1)
			{
			}

			operator char *() const { return &data[0]; }

			mutable std::vector<char> data;
		};

		// substitute stdin, stdout and stderr with pipes
		if (::dup2(pipe_in[READ], STDIN_FILENO) == -1)
			throw std::system_error(errno, std::system_category());
		if (::dup2(pipe_out[WRITE], STDOUT_FILENO) == -1)
			throw std::system_error(errno, std::system_category());
		if (::dup2(pipe_err[WRITE], STDERR_FILENO) == -1)
			throw std::system_error(errno, std::system_category());

		close_all();

		// prepare parameters for 'execvp'
		std::vector<mutable_string> tm{args.begin(), args.end()};
		std::vector<char *> params(tm.begin(), tm.end());
		params.push_back(nullptr);

		if (execvp(params[0], &params[0]) != -1)
			throw std::system_error(errno, std::system_category());
	}
};

subprocess & operator>>(subprocess & source, subprocess & destination)
{
	return source(destination());
}
}

#endif
