#include "pch.h"
#include "StopWatch.h"

///////////////////////////////////////////////////////////////////////////
// http_client
///////////////////////////////////////////////////////////////////////////

const utility::char_t kApiKey[] = U("W72R8P1Y8LK6SKFB");

const char* const kSimboli[] = {
	"MSFT",
	"FB",
	"AAPL",
	"GOOG",
	"EBAY",
	nullptr
};

const utility::char_t kFormat[] = U("json"); // csv|json
const utility::char_t kOutputSize[] = U("compact"); // compact|full

static std::string FormatDuration(std::chrono::steady_clock::duration duration)
{
	const auto total_millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	const auto seconds = total_millis.count() / 1000;
	const auto millis = total_millis.count() % 1000;

	std::stringstream ss;
	ss << seconds << '.' << std::setfill('0') << std::setw(3) << millis
		<< std::setw(0) << 's';
	return ss.str();
}

static pplx::task<void> ScaricaQuotazioni(const std::string& simbolo)
{
	auto fileBuffer = std::make_shared<pplx::streams::streambuf<uint8_t>>();

	StopWatch watch;

	utility::string_t simbolo_u = utility::conversions::to_string_t(simbolo);

	// apri il file in scrittura
	return pplx::streams::file_buffer<uint8_t>::open(
		simbolo_u + U('.') + kFormat,
		std::ios::out)
	// fai la richiesta http
	.then([=](pplx::streams::streambuf<uint8_t> outFile)
		-> pplx::task<web::http::http_response>	{
		*fileBuffer = outFile;
		web::http::client::http_client client(U("https://www.alphavantage.co/"));
		auto uri = web::uri_builder(U("/query"))
			.append_query(U("function"), U("TIME_SERIES_DAILY_ADJUSTED"))
			.append_query(U("symbol"), simbolo_u)
			.append_query(U("outputsize"), kOutputSize)
			.append_query(U("apikey"), kApiKey)
			.append_query(U("datatype"), kFormat);
		return client.request(web::http::methods::GET, uri.to_string());
	})
	// scrivi la risposta su file
	.then([=](web::http::http_response response)
		-> pplx::task<size_t> {
		std::cout << simbolo << ": status code=" << response.status_code() << '\n';
		return response.body().read_to_end(*fileBuffer);
	})
	// chiudi il file
	.then([=](size_t) {
		return fileBuffer->close();
	})
	// stampa tempo impiegato
	.then([=]() {
		std::cout << simbolo << ": tempo="
			<< FormatDuration(watch.Duration()) << '\n';
	});
}

static void ScaricaQuotazioniSequenziale()
{
	for (const auto* simbolo_ptr = &kSimboli[0]; *simbolo_ptr != nullptr; ++simbolo_ptr) {
		ScaricaQuotazioni(*simbolo_ptr).wait();
	}
}

static void ScaricaQuotazioniParallelo()
{
	std::vector<pplx::task<void>> tasks;
	for (const auto* simbolo_ptr = &kSimboli[0]; *simbolo_ptr != nullptr; ++simbolo_ptr) {
		tasks.push_back(ScaricaQuotazioni(*simbolo_ptr));
	}

	pplx::when_all(std::begin(tasks), std::end(tasks)).wait();
}


///////////////////////////////////////////////////////////////////////////
// websockets
///////////////////////////////////////////////////////////////////////////

static void RispondiConEco()
{
	namespace wsc = web::websockets::client;

	web::uri address(U("ws://echo.websocket.org/"));
	wsc::websocket_client client;
	std::cout << "Provo a connettere\n";
	client.connect(address)
	.then([&]() {
		std::cout << "Invio un messaggio di testo\n";
		wsc::websocket_outgoing_message send_msg;
		send_msg.set_utf8_message("Ciao mondo!");
		return client.send(std::move(send_msg));
	})
	.then([&]() {
		std::cout << "Provo a ricevere\n";
		return client.receive();
	})
	.then([&](const wsc::websocket_incoming_message& recv_msg) {
		std::cout << "Leggo il messaggio\n";
		return recv_msg.extract_string();
	})
	.then([&](const std::string& body) {
		std::cout << "Ricevuto: '" << body << "'\n";
		std::cout << "Provo a disconnettere\n";
		return client.close();
	})
	.then([]() {
		std::cout << "Disconnesso\n";
	})
	.then([](pplx::task<void> t) {
		try	{
			t.get();
		}
		catch (const wsc::websocket_exception& ex) {
			std::cout << ex.what();
		}
	})
	.wait();
}

int main()
{
	StopWatch tempo_totale;
	//ScaricaQuotazioniSequenziale();
	ScaricaQuotazioniParallelo();
	std::cout << "Tempo totale: " << FormatDuration(tempo_totale.Duration()) << '\n';	
	std::cout << "-----------------\n";
	RispondiConEco();
}
