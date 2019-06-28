#ifndef __TESTBENCH_H_
#define __TESTBENCH_H_

#include <iostream>
#include <vector>
#include <array>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <string>
#include <set>
#include <sstream>

#ifdef CPP17
#include <chrono>
namespace ch = std::chrono;
#else 
#include <sys/time.h>
timeval GetUnixTimeVal() {
	struct timeval TimeVal;
	struct timezone TimeZone;
	gettimeofday(&TimeVal, &TimeZone);
	return TimeVal;
}
#endif

namespace {
enum TestType : int {
	Add = 0,
	Get,
	Del,
	Iterate,
	E_LAST
};
#define TestTypeN ((size_t)(TestType::E_LAST))
}


struct TestInfo {
	TestType type;

	TestInfo(TestType inType)
		: type(inType) {};
};

// All the data our Benchmark should store for 1 test.
struct TestData {
	long long Time;

	int testsIncluded;

	TestData() 
		: Time(0)
		, testsIncluded(0) {}

	TestData(long long time)
		: Time(time) {}
};

void operator+=(TestData& rhs, const TestData& lhs) {
	rhs.Time += lhs.Time;
	rhs.testsIncluded++;
}

bool operator<(TestData& rhs, const TestData& lhs) {
	return rhs.Time < lhs.Time;
}

// Struct to hold the benchmark results.
// Implementation can switch between chrono / unix time through defining USE_CHRONO.

struct BenchmarkResults {
private:
	std::vector<TestData> ImplTime;
	std::vector<TestData> LedaTime;
	std::vector<long long> BlockReads;
	std::vector<TestInfo> TestList;
	bool OutputHumanReadable;
public:
	BenchmarkResults()
		: OutputHumanReadable(true) {}

	long long CurrentBenchBlocks;

	void SetTextOutput(bool OutputText = true) {
		OutputHumanReadable = OutputText;
	}

private:
	static std::string TimestepStr() {
		return " ms";
	}

#ifdef CPP17
	ch::time_point<ch::system_clock> StartTime;
	void RestartTimer() {
		StartTime = ch::system_clock::now();
	}

	long long GetCurrent() const {
		return ch::duration_cast<ch::milliseconds>(ch::system_clock::now() - StartTime).count();
	}
#else
	struct timeval StartTime;

	void RestartTimer() {
		StartTime = GetUnixTimeVal();
	}

	long long GetCurrent() const {
        struct timeval EndTime = GetUnixTimeVal();
		long long micros;
		if (EndTime.tv_sec == StartTime.tv_sec) {
			micros = EndTime.tv_usec - StartTime.tv_usec;
		}
		else {
			micros = (EndTime.tv_sec - StartTime.tv_sec - 1) * 1000000 + (1000000 - StartTime.tv_usec) + EndTime.tv_usec;
		}
		return micros / 1000;
	}
#endif

public:

	// Internal,  formats and prints a line with 2 times and their difference.
	void PrintBenchLine(const std::string& Title, TestData Impl, TestData Leda, long long Blocks) {
		std::string BlockStr = Blocks > 0 ? "\tBlocks Accessed: " + std::to_string(Blocks / 1000) + "k" : "";

		if (OutputHumanReadable) {
			std::cout << std::right;
			std::cout << "# " << std::setw(18) << std::left << Title << " Impl: " << std::right << std::setw(7) << Impl.Time << TimestepStr() << " | "
				"LEDA: " << std::setw(7) << Leda.Time << TimestepStr() << " => Diff: " << std::setw(6) << Leda.Time - Impl.Time << " " << BlockStr << "\n";
		}
		else {
			std::cout << Title << ", " << Impl.Time << ", " << Leda.Time << ", " << std::to_string(Blocks) << "\n";
		}
	}

public:
	void Reset(unsigned int Elements, size_t NodeBytes) {
		ImplTime.clear();
		LedaTime.clear();
		BlockReads.clear();
		TestList.clear();
		CurrentBenchBlocks = 0;

		if (OutputHumanReadable) {
			std::cout << "\n>>> Elements: " << Elements << " | Memsize: " << NodeBytes << " B <<<\n\n";
		}
		else {
			std::cout << "\n" << Elements << ", Impl, Leda, Blocks\n";
		}
		
	}

	void StartTest() {
		CurrentBenchBlocks = 0;
		RestartTimer();
	}

	void StopLeda() {
		long long Duration = GetCurrent();
		LedaTime.push_back(TestData(Duration));
	}

    void StopImpl() {
		long long Duration = GetCurrent();
		ImplTime.push_back(TestData(Duration));
		BlockReads.push_back(CurrentBenchBlocks);
	}

	// Print the last added test.
	void PrintLast(TestType Info, const std::string& Title) {
		TestList.push_back(TestInfo(Info));
		size_t Index = ImplTime.size() - 1;
		PrintBenchLine(Title, ImplTime[Index], LedaTime[Index], BlockReads[Index]);
	}

	// Calculate and print total stats.
	void Print() {
        bool ContainsInvalidResult = false;
		TestData ImplTotal;
		TestData LedaTotal;
		long long BlocksTotal = 0;

		std::array<TestData, TestTypeN> ImplPerType;
		std::array<TestData, TestTypeN> LedaPerType;
		std::array<long long, TestTypeN> BlocksPerType = {0};



		for (int i = 0; i < ImplTime.size(); ++i) {
			ImplTotal += ImplTime[i];
			LedaTotal += LedaTime[i];
			BlocksTotal += BlockReads[i];

			ImplPerType[(int)(TestList[i].type)] += ImplTime[i];
			LedaPerType[(int)(TestList[i].type)] += LedaTime[i];
			BlocksPerType[(int)(TestList[i].type)] += BlockReads[i];
		}

		if (OutputHumanReadable) {
			std::cout << "\n";
		}

		PrintBenchLine("# Get", 
					   ImplPerType[(int)(TestType::Get)], 
					   LedaPerType[(int)(TestType::Get)],
					   BlocksPerType[(int)(TestType::Get)]);

		PrintBenchLine("# Add", 
					   ImplPerType[(int)(TestType::Add)],
					   LedaPerType[(int)(TestType::Add)],
					   BlocksPerType[(int)(TestType::Add)]);

		PrintBenchLine("# Del", 
					   ImplPerType[(int)(TestType::Del)],
					   LedaPerType[(int)(TestType::Del)], 
					   BlocksPerType[(int)(TestType::Del)]);

		PrintBenchLine("# Iter", 
					   ImplPerType[(int)(TestType::Iterate)], 
					   LedaPerType[(int)(TestType::Iterate)], 
					   BlocksPerType[(int)(TestType::Iterate)]);

		PrintBenchLine("# Totals", ImplTotal, LedaTotal, BlocksTotal);
	}
};

// utility to convert to dotted number.
struct dotted : std::numpunct<char> {
	char do_thousands_sep()   const { 
		return '.'; 
	} 

	std::string do_grouping() const { 
		return "\3"; 
	}

	static void imbue(std::ostream& os) {
		os.imbue(std::locale(os.getloc(), new dotted));
	}

	static std::string str(long long Value) {
		std::stringstream ss;
		dotted::imbue(ss);
		ss << Value;
		return ss.str();
	}
};

struct AggregateTimer {
	long long totalNanos;
	long long timesHit;

	void Stop() {
		totalNanos += GetCurrent();
	}

	AggregateTimer()
		: totalNanos(0)
		, timesHit(0) {}

#ifdef CPP17
	ch::time_point<ch::system_clock> StartTime;

	void Start() {
		timesHit++;
		StartTime = ch::system_clock::now();
	}

	long long GetCurrent() const {
		return ch::duration_cast<ch::nanoseconds>(ch::system_clock::now() - StartTime).count();
	}
#else 
	struct timeval StartTime;

	void Start() {
		timesHit++;
		StartTime = GetUnixTimeVal();
	}

	long long GetCurrent() const {
		struct timeval EndTime = GetUnixTimeVal();
		long long micros;
		if (EndTime.tv_sec == StartTime.tv_sec) {
			micros = EndTime.tv_usec - StartTime.tv_usec;
		}
		else {
			micros = (EndTime.tv_sec - StartTime.tv_sec - 1) * 1000000 + (1000000 - StartTime.tv_usec) + EndTime.tv_usec;
		}
		return micros * 1000;
	}
#endif

	void Print(const std::string& Title) const {
		if (timesHit == 0) {
			return;
		}
		std::cout << "Timer # " << std::setw(12) << Title << " hits: " << std::setw(10) << dotted::str(timesHit) 
			<< " | " << std::setw(16) << dotted::str(totalNanos) << "ns ( avg:" << std::setw(11) 
			<< dotted::str(totalNanos / timesHit) << "ns )\n";
	}

	struct Scope {
		AggregateTimer& timer;
		Scope(AggregateTimer& parent)
			: timer(parent) {
			timer.Start();
		}

		~Scope() {
			timer.Stop();
		}
	};
};


#ifdef DECLARE_EXTERN_TESTBENCH_VARS
# if COUNT_BLOCKS
#  define INCR_BLOCKS() do{ ++bench.CurrentBenchBlocks; }while(0)
# endif
extern AggregateTimer timer;
extern BenchmarkResults bench;
#endif

#endif //__TESTBENCH_H_