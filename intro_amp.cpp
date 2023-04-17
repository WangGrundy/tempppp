// Data Structures and Algorithms II : Intro to AMP and benchmarking exercise
// Ruth Falconer  <r.falconer@abertay.ac.uk>
// Adapted from C++ AMP book http://ampbook.codeplex.com/license.

/*************************To do in lab ************************************/
//Change size of the vector/array
//Compare Debug versus Release modes
//Add more work to the loop until the GPU is faster than CPU
//Compare double versus ints via templating  

#define _SILENCE_AMP_DEPRECATION_WARNINGS

#include <chrono>
#include <iostream>
#include <iomanip>
#include <amp.h>
#include <time.h>
#include <string>
#include <array>
#include <assert.h>

#include <thread>

//#define SIZE 1<<24 // same as 2^24
#define SIZE 1024 * 10000
const int TS = 1024;

// Need to access the concurrency libraries 
using namespace concurrency;

// Import things we need from the standard library
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::cout;
using std::endl;

// Define the alias "the_clock" for the clock type we're going to use.
typedef std::chrono::steady_clock the_serial_clock;
typedef std::chrono::steady_clock the_amp_clock;

void report_accelerator(const accelerator a)
{
	const std::wstring bs[2] = { L"false", L"true" };
	std::wcout << ": " << a.description << " "
		<< endl << "       device_path                       = " << a.device_path
		<< endl << "       dedicated_memory                  = " << std::setprecision(4) << float(a.dedicated_memory) / (1024.0f * 1024.0f) << " Mb"
		<< endl << "       has_display                       = " << bs[a.has_display]
		<< endl << "       is_debug                          = " << bs[a.is_debug]
		<< endl << "       is_emulated                       = " << bs[a.is_emulated]
		<< endl << "       supports_double_precision         = " << bs[a.supports_double_precision]
		<< endl << "       supports_limited_double_precision = " << bs[a.supports_limited_double_precision]
		<< endl;
}
// List and select the accelerator to use
void list_accelerators()
{
	//get all accelerators available to us and store in a vector so we can extract details
	std::vector<accelerator> accls = accelerator::get_all();

	// iterates over all accelerators and print characteristics
	for (int i = 0; i < accls.size(); i++)
	{
		accelerator a = accls[i];
		report_accelerator(a);

	}

	//Use default accelerator
	accelerator a = accelerator(accelerator::default_accelerator);
	std::wcout << " default acc = " << a.description << endl;
	cout << endl;
} // list_accelerators

// query if AMP accelerator exists on hardware
void query_AMP_support()
{
	std::vector<accelerator> accls = accelerator::get_all();
	if (accls.empty())
	{
		cout << "No accelerators found that are compatible with C++ AMP" << std::endl;
	}
	else
	{
		cout << "Accelerators found that are compatible with C++ AMP" << std::endl;
		list_accelerators();
	}
} // query_AMP_support

 // Unaccelerated CPU element-wise addition of arbitrary length vectors using C++ 11 container vector class

void vector_add(const int size, const std::vector<double>& v1, const std::vector<double>& v2, std::vector<double>& v3)
{
	//start clock for serial version
	the_serial_clock::time_point start = the_serial_clock::now();
	// loop over and add vector elements
	for (int i = 0; i < v3.size(); i++)
	{
		v3[i] = v2[i] + v1[i];
		
	}
	the_serial_clock::time_point end = the_serial_clock::now();
	//Compute the difference between the two times in milliseconds
	auto time_taken = duration_cast<milliseconds>(end - start).count();
	cout << "Adding the vectors serially usin the CPU " << time_taken << " ms." << endl;
} // vector_add

  // Accelerated  element-wise addition of arbitrary length vectors using C++ 11 container vector class #
void vector_add_amp(const int size, const std::vector<double>& v1, const std::vector<double>& v2, std::vector<double>& v3)
{
	concurrency::array_view<const double> av1(size, v1);
	concurrency::array_view<const double> av2(size, v2);
	extent<1> e(size);
	concurrency::array_view<double> av3(e, v3);
	av3.discard_data();
	// start clock for GPU version after array allocation
	the_amp_clock::time_point start = the_amp_clock::now();
	// It is wise to use exception handling here - AMP can fail for many reasons
	// and it useful to know why (e.g. using double precision when there is limited or no support)
	try
	{
		concurrency::parallel_for_each(av3.extent, [=](concurrency::index<1> idx)  restrict(amp)
			{
				av3[idx] = av1[idx] + av2[idx];
				
			});
		av3.synchronize();
	}
	catch (const Concurrency::runtime_exception & ex)
	{
		MessageBoxA(NULL, ex.what(), "Error", MB_ICONERROR);
	}
	// Stop timing
	the_amp_clock::time_point end = the_amp_clock::now();
	// Compute the difference between the two times in milliseconds
	auto time_taken = duration_cast<milliseconds>(end - start).count();

	cout << "Adding the vectors using AMP (data transfer and compute) takes " << time_taken << " ms." << endl;
} // vector_add_amp

void stdArrayAdd(const int size, const double a1[], const double a2[], double a3[])
{
	the_amp_clock::time_point start = the_amp_clock::now();

	for (int i = 0; i < size; i++) {
		a3[i] = a1[i] + a2[i];
		//std::cout << a3[i];
	}

	the_amp_clock::time_point end = the_amp_clock::now();
	auto time_taken = duration_cast<milliseconds>(end - start).count();
	cout << "ADDING ARRAY IN SERIAL " << time_taken << " ms." << endl;
} 

template <typename T>
void AddTemplateArray(const int size, const T a1[], const T a2[], T a3[])
{
	the_amp_clock::time_point start = the_amp_clock::now();

	for (int i = 0; i < size; i++) {
		a3[i] = a1[i] + a2[i];
	}	

	the_amp_clock::time_point end = the_amp_clock::now();
	auto time_taken = duration_cast<milliseconds>(end - start).count();
	cout << "ADDING ARRAY IN SERIAL using templates : " << time_taken << " ms." << endl;
}

void vector_add_amp_tile(const int size, const std::vector<double>& v1, const std::vector<double>& v2, std::vector<double>& v3)
{

	concurrency::array_view<const double> av1(TS, v1);
	concurrency::array_view<const double> av2(TS, v2);
	extent<1> e(TS);
	concurrency::array_view<double> av3(e, v3);

	av3.discard_data();
	// start clock for GPU version after array allocation
	the_amp_clock::time_point start = the_amp_clock::now();
	// It is wise to use exception handling here - AMP can fail for many reasons
	// and it useful to know why (e.g. using double precision when there is limited or no support)
	try
	{
		concurrency::parallel_for_each(av3.extent.tile<TS>(), [=](concurrency::tiled_index<TS> t_idx)
			restrict(amp)

			{
				av3[t_idx.global] = av1[t_idx] + av2[t_idx];

			});
		av3.synchronize();
	}
	catch (const Concurrency::runtime_exception& ex)
	{
		MessageBoxA(NULL, ex.what(), "Error", MB_ICONERROR);
	}
	// Stop timing
	the_amp_clock::time_point end = the_amp_clock::now();
	// Compute the difference between the two times in milliseconds
	auto time_taken = duration_cast<milliseconds>(end - start).count();

	cout << "Adding the vectors using AMP (data transfer and compute) takes " << time_taken << " ms." << endl;
} // vector_add_amp

void vector_add_amp_tile_static(const int size, const std::vector<double>& v1, const std::vector<double>& v2, std::vector<double>& v3)
{

	concurrency::array_view<const double> av1(TS, v1);
	concurrency::array_view<const double> av2(TS, v2);
	extent<1> e(TS);
	concurrency::array_view<double> av3(e, v3);

	av3.discard_data();
	the_amp_clock::time_point start = the_amp_clock::now();
	try
	{
		concurrency::parallel_for_each(av3.extent.tile<TS>(), [=](concurrency::tiled_index<TS> t_idx)
			restrict(amp)

			{
				tile_static double locationA[TS];
				tile_static double locationB[TS];

				locationA[t_idx.local[0]] = av1[t_idx.global]; //GLOBAL TO LOCAL (av1 global - > locationA local)
				locationB[t_idx.local[0]] = av2[t_idx.global]; 

				t_idx.barrier.wait();

				av3[t_idx.global] = locationA[t_idx.local[0]] + locationB[t_idx.local[0]]; 

			});
		av3.synchronize();

	}
	catch (const Concurrency::runtime_exception& ex)
	{
		MessageBoxA(NULL, ex.what(), "Error", MB_ICONERROR);
	}
	// Stop timing
	the_amp_clock::time_point end = the_amp_clock::now();
	// Compute the difference between the two times in milliseconds
	auto time_taken = duration_cast<milliseconds>(end - start).count();

	cout << "Adding the vectors using AMP (data transfer and compute) takes " << time_taken << " ms." << endl;
} // vector_add_amp
 
int main(int argc, char* argv[])
{
	// Check AMP support
	query_AMP_support();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	//fill the arrays - try ints versus floats & double -- change via templating
	std::vector<double> v1(SIZE, 1.0);
	std::vector<double> v2(SIZE, 2.0);
	std::vector<double> v3(SIZE, 0.0);

	static double a1[SIZE]; 
	static double a2[SIZE]; 
	static double a3[SIZE];

	for (int i = 0; i < SIZE; i++) {
		a1[i] = 1.0;
		a2[i] = 2.0;
		a3[i] = 0.0;
	}

	std::cout << "BEFORE: ";
	for (int i = 0; i < 10; i++) {
		
		std::cout << v3[i] << std::endl;
	}

	//vector_add(SIZE, v1, v2, v3); //450ms
	//stdArrayAdd(SIZE, a1, a2, a3); //26
	//AddTemplateArray(SIZE, a1, a2, a3); //28ms
	//vector_add_amp_tile(SIZE, v1, v2, v3); //4ms
	//vector_add_amp_tile_static(SIZE, v1, v2, v3); //4 ms

	std::cout << "AFTER: ";
	for (int i = 0; i < 10; i++) {
		std::cout << v3[i] << std::endl;
	}

	return 0;
} // main




