#include <litehtml.h>
#include "test_container.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>

using namespace std;
using namespace litehtml;

static string read_file(const string& path)
{
	ifstream f(path);
	if(!f.is_open())
	{
		cerr << "ERROR: Cannot open file: " << path << endl;
		exit(1);
	}
	stringstream ss;
	ss << f.rdbuf();
	return ss.str();
}

struct BenchResult
{
	string name;
	double mean_us, median_us, min_us, max_us, stddev_us;
	int	   iterations;
};

static BenchResult compute_stats(const string& name, const vector<double>& times_us)
{
	BenchResult r;
	r.name				  = name;
	r.iterations		  = (int) times_us.size();
	vector<double> sorted = times_us;
	sort(sorted.begin(), sorted.end());
	r.min_us	  = sorted.front();
	r.max_us	  = sorted.back();
	r.median_us	  = sorted[sorted.size() / 2];
	r.mean_us	  = accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size();
	double sum_sq = 0;
	for(double t : sorted)
	{
		double d  = t - r.mean_us;
		sum_sq	 += d * d;
	}
	r.stddev_us = sqrt(sum_sq / sorted.size());
	return r;
}

static void print_result(const BenchResult& r)
{
	printf("  %-35s %8.0f us  (median: %8.0f, min: %8.0f, max: %8.0f, n=%d)\n", r.name.c_str(), r.mean_us, r.median_us,
		   r.min_us, r.max_us, r.iterations);
}

static void print_summary_table(const vector<BenchResult>& results)
{
	printf("\n╔═══════════════════════════════════════╦══════════════╦══════════════╦══════════════╗\n");
	printf("║ %-37s ║ %12s ║ %12s ║ %12s ║\n", "Benchmark", "Mean (us)", "Median (us)", "Min (us)");
	printf("╠═══════════════════════════════════════╬══════════════╬══════════════╬══════════════╣\n");
	for(const auto& r : results)
		printf("║ %-37s ║ %12.0f ║ %12.0f ║ %12.0f ║\n", r.name.c_str(), r.mean_us, r.median_us, r.min_us);
	printf("╚═══════════════════════════════════════╩══════════════╩══════════════╩══════════════╝\n");
}

int main(int argc, char* argv[])
{
	int iterations = 200;
	if(argc > 1)
	{
		iterations = atoi(argv[1]);
		if(iterations < 1)
			iterations = 200;
	}

	printf("=== litehtml Benchmark ===\n");
	printf("Iterations: %d\n\n", iterations);

	string html		 = read_file("test_page.html");
	string css		 = read_file("test_styles.css");

	// Combine CSS inline
	string full_html = html;
	size_t link_pos	 = full_html.find("<link rel=\"stylesheet\"");
	if(link_pos != string::npos)
	{
		size_t end_pos = full_html.find(">", link_pos);
		if(end_pos != string::npos)
			full_html.replace(link_pos, end_pos - link_pos + 1, "<style>\n" + css + "\n</style>");
	}

	printf("Document: %zu bytes (%zu HTML + %zu CSS)\n\n", full_html.size(), html.size(), css.size());

	test_container		container(1024, 768, ".");
	vector<BenchResult> all_results;

	// 1. createFromString (one-time)
	{
		printf("[1] createFromString...\n");
		vector<double> times;
		for(int i = 0; i < iterations; i++)
		{
			auto t0	 = chrono::high_resolution_clock::now();
			auto doc = document::createFromString(full_html.c_str(), &container);
			auto t1	 = chrono::high_resolution_clock::now();
			times.push_back(chrono::duration<double, micro>(t1 - t0).count());
		}
		auto r = compute_stats("createFromString", times);
		print_result(r);
		all_results.push_back(r);
	}

	// 2. render (same width)
	{
		printf("[2] render (same width)...\n");
		auto		   doc = document::createFromString(full_html.c_str(), &container);
		vector<double> times;
		for(int i = 0; i < iterations; i++)
		{
			auto t0 = chrono::high_resolution_clock::now();
			doc->render(1024);
			auto t1 = chrono::high_resolution_clock::now();
			times.push_back(chrono::duration<double, micro>(t1 - t0).count());
		}
		auto r = compute_stats("render (same width)", times);
		print_result(r);
		all_results.push_back(r);
	}

	// 3. re-render (resize)
	{
		printf("[3] re-render (resize)...\n");
		auto doc = document::createFromString(full_html.c_str(), &container);
		doc->render(1024);
		vector<double> times;
		int			   widths[] = {800, 1024, 1280, 960, 1440, 768, 1200, 640, 1366, 1920};
		for(int i = 0; i < iterations; i++)
		{
			auto t0 = chrono::high_resolution_clock::now();
			doc->render(widths[i % 10]);
			auto t1 = chrono::high_resolution_clock::now();
			times.push_back(chrono::duration<double, micro>(t1 - t0).count());
		}
		auto r = compute_stats("re-render (resize)", times);
		print_result(r);
		all_results.push_back(r);
	}

	// 4. hover simulation
	{
		printf("[4] hover simulation...\n");
		auto doc = document::createFromString(full_html.c_str(), &container);
		doc->render(1024);
		vector<double> times;
		for(int i = 0; i < iterations; i++)
		{
			int				 x = 200 + (i * 37) % 600;
			int				 y = 50 + (i * 73) % 700;
			position::vector redraw_boxes;
			auto			 t0		 = chrono::high_resolution_clock::now();
			bool			 changed = doc->on_mouse_over(x, y, x, y, redraw_boxes);
			if(changed)
				doc->render(1024);
			auto t1 = chrono::high_resolution_clock::now();
			times.push_back(chrono::duration<double, micro>(t1 - t0).count());
		}
		auto r = compute_stats("hover simulation", times);
		print_result(r);
		all_results.push_back(r);
	}

	// 5. hover forced change (alternating elements)
	{
		printf("[5] hover forced change...\n");
		auto doc = document::createFromString(full_html.c_str(), &container);
		doc->render(1024);
		vector<double> times;
		int			   positions[][2] = {
			   {100, 100},
			   {500, 400}
		 };
		for(int i = 0; i < iterations; i++)
		{
			int				 idx = i % 2;
			position::vector redraw_boxes;
			auto			 t0		 = chrono::high_resolution_clock::now();
			bool			 changed = doc->on_mouse_over(positions[idx][0], positions[idx][1], positions[idx][0],
														  positions[idx][1], redraw_boxes);
			if(changed)
				doc->render(1024);
			auto t1 = chrono::high_resolution_clock::now();
			times.push_back(chrono::duration<double, micro>(t1 - t0).count());
		}
		auto r = compute_stats("hover forced change", times);
		print_result(r);
		all_results.push_back(r);
	}

	// 6. flexbox stress test
	{
		printf("[6] flexbox stress test (100x50 grid)...\n");
		
		string stress_html = "<!DOCTYPE html><html><head><style>"
			".container { display: flex; flex-direction: column; width: 1000px; height: 100%; }"
			".row { display: flex; flex-direction: row; flex: 1; margin: 2px; }"
			".item { flex: 1; margin: 1px; padding: 2px; border: 1px solid black; background-color: #eee; }"
			".subitem { display: flex; flex: 1; background-color: #ddd; }"
			"</style></head><body><div class=\"container\">";
		
		for (int i = 0; i < 100; i++) {
			stress_html += "<div class=\"row\">";
			for (int j = 0; j < 50; j++) {
				stress_html += "<div class=\"item\"><div class=\"subitem\">Item " + std::to_string(i) + "-" + std::to_string(j) + "</div></div>";
			}
			stress_html += "</div>";
		}
		stress_html += "</div></body></html>";
		
		vector<double> times_parse;
		vector<double> times_render;
		
		for(int i = 0; i < std::max(1, iterations / 20); i++) // Fewer iterations, it's a heavy test
		{
			auto t0 = chrono::high_resolution_clock::now();
			auto doc = document::createFromString(stress_html.c_str(), &container);
			auto t1 = chrono::high_resolution_clock::now();
			doc->render(1024);
			auto t2 = chrono::high_resolution_clock::now();
			
			times_parse.push_back(chrono::duration<double, micro>(t1 - t0).count());
			times_render.push_back(chrono::duration<double, micro>(t2 - t1).count());
		}
		auto r_parse = compute_stats("flex stress - parse", times_parse);
		auto r_render = compute_stats("flex stress - render", times_render);
		print_result(r_parse);
		print_result(r_render);
		all_results.push_back(r_parse);
		all_results.push_back(r_render);
	}

	print_summary_table(all_results);
	printf("\nDone.\n");
	return 0;
}
