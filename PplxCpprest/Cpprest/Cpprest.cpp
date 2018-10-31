#include "pch.h"

#include "StopWatch.h"

#include <cpprest/http_client.h>
#include <cpprest/uri.h>
#include <cpprest/filestream.h>
#include <cpprest/asyncrt_utils.h>
#include <pplx/pplxtasks.h>

#include <chrono>
#include <iostream>
#include <iomanip>
#include <memory>
#include <utility>
#include <string>
#include <vector>

#include <cstdint>

const utility::char_t kApiKey[] = U("W72R8P1Y8LK6SKFB");

const utility::char_t* const kSimboli[] = {
	U("MSFT"),
	U("FB"),
	U("AAPL"),
	U("GOOG"),
	U("EBAY"),
	nullptr
};

const utility::char_t kFormat[] = U("json"); // csv|json
const utility::char_t kOutputSize[] = U("compact"); // compact|full

static utility::string_t FormatDuration(std::chrono::steady_clock::duration duration)
{
	auto total_millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	utility::stringstream_t ss;
	auto seconds = total_millis.count() / 1000;
	auto millis = total_millis.count() % 1000;

	ss << seconds << U('.') << std::setfill(U('0')) << std::setw(3) << millis
		<< std::setw(0) << U('s');
	return ss.str();
}

static pplx::task<void> ScaricaQuotazioni(const utility::string_t& simbolo)
{
	auto fileBuffer = std::make_shared<pplx::streams::streambuf<uint8_t>>();

	StopWatch watch;

	// apri il file in scrittura
	return pplx::streams::file_buffer<uint8_t>::open(
		simbolo + U('.') + kFormat,
		std::ios::out)
	// fai la richiesta http
	.then([=](pplx::streams::streambuf<uint8_t> outFile)
		-> pplx::task<web::http::http_response>	{
		*fileBuffer = outFile;
		web::http::client::http_client client(U("https://www.alphavantage.co/"));
		auto uri = web::uri_builder(U("/query"))
			.append_query(U("function"), U("TIME_SERIES_DAILY_ADJUSTED"))
			.append_query(U("symbol"), utility::conversions::to_string_t(simbolo))
			.append_query(U("outputsize"), kOutputSize)
			.append_query(U("apikey"), kApiKey)
			.append_query(U("datatype"), kFormat);
		return client.request(web::http::methods::GET, uri.to_string());
	})
	// scrivi la risposta su file
	.then([=](web::http::http_response response)
		-> pplx::task<size_t> {
		ucout << simbolo << U(": status code=") << response.status_code() << U('\n');
		return response.body().read_to_end(*fileBuffer);
	})
	// chiudi il file
	.then([=](size_t) {
		return fileBuffer->close();
	})
	// stampa tempo impiegato
	.then([=]() {
		ucout << simbolo << U(": tempo=")
			<< FormatDuration(watch.Duration()) << U('\n');
	});
}

static void Sequenziale()
{
	for (const auto* simbolo_ptr = &kSimboli[0]; *simbolo_ptr != nullptr; ++simbolo_ptr) {
		ScaricaQuotazioni(*simbolo_ptr).wait();
	}
}

static void Parallelo()
{
	std::vector<pplx::task<void>> tasks;
	for (const auto* simbolo_ptr = &kSimboli[0]; *simbolo_ptr != nullptr; ++simbolo_ptr) {
		tasks.push_back(ScaricaQuotazioni(*simbolo_ptr));
	}

	pplx::when_all(std::begin(tasks), std::end(tasks)).wait();
}


int main()
{
	StopWatch tempo_totale;
	//Sequenziale();
	Parallelo();
	ucout << U("Tempo totale: ") << FormatDuration(tempo_totale.Duration()) << U('\n');

	utility::string_t in;
	ucin >> in;
}
