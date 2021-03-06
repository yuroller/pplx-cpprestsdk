#include "pch.h"

namespace wh = web::http;

static const char kIndexPage[] =
"<html>\n"
"  <head>\n"
"    <title>Esempio HttpListener</title>\n"
"  </head>\n"
"  <body>\n"
"    <p>Ciao da HttpListener</p>\n"
"  </body>\n"
"</html>\n";

class HttpListener
{
public:
	HttpListener(wh::uri address)
		: m_listener(std::move(address))
	{
		// void(*handler)(http_request)
		m_listener.support(wh::methods::GET,
			std::bind(&HttpListener::handle_get, this, std::placeholders::_1));
		m_listener.support(wh::methods::POST,
			std::bind(&HttpListener::handle_post, this, std::placeholders::_1));
	}

	pplx::task<void> open() { return m_listener.open(); }
	pplx::task<void> close() { return m_listener.close(); }

private:
	void handle_get(wh::http_request message)
	{
		ucout << U("Ricevuto:\n") << message.to_string() << U('\n');

		auto paths = wh::uri::split_path(wh::uri::decode(message.relative_uri().path()));
		if (paths.empty())
		{
			message.reply(wh::status_codes::OK, kIndexPage,
				"text/html; charset=utf-8");
			return;
		}

		message.reply(wh::status_codes::NotFound);
	}

	void handle_post(wh::http_request message)
	{}

	wh::experimental::listener::http_listener m_listener;
};

static void AspettaInvio(const utility::string_t& messaggio)
{
	ucout << U("Premi [invio] per ") << messaggio << U('\n');
	utility::string_t line;
	std::getline(ucin, line);
}

int main()
{
	auto address = web::uri_builder(U("http://localhost:33223/"))
		.append_path(U("esempio"));
	HttpListener listener(address.to_uri());
	ucout << U("Mi metto in ascolto su: ") << address.to_string() << U('\n');
	listener.open().wait();
	AspettaInvio(U("chiudere il server"));
	ucout << U("Provo a chiudere il server\n");
	listener.close().wait();
	ucout << U("Server chiuso\n");
}
