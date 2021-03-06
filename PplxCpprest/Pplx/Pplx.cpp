// PplxCpprest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "ppltasks_extra.h"

static void FutureNonSiCompongono()
{	
	std::future<int> computation = std::async([]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::cout << "Async ha eseguito sul thread "
			<< std::this_thread::get_id() << '\n';
		return 3;
	});

	std::cout << "Aspetto il risultato sul thread "
		<< std::this_thread::get_id() << '\n';
	int res = computation.get();
	std::cout << "risultato=" << res << '\n';
}

static void TaskEsegueAppenaCreato()
{
	pplx::task<void> task = pplx::create_task([]() {
		std::cout << "Dentro al task!\n";
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << "Prima dell'attesa del completamento\n";
	task.wait();
	std::cout << "Dopo il completamento\n";
}

static void TaskLanciatoAComando()
{
	auto lazy = []() {
		return pplx::create_task([]() {
			std::cout << "Dentro al task!\n";
		});
	};

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << "Prima di della creazione\n";
	lazy().wait();
	std::cout << "Dopo il completamento\n";
}

static void TaskRestituisceValore()
{
	pplx::task<int> task = pplx::create_task([]() {
		return 42;
	});

	std::cout << "Task restituisce " << task.get() << '\n';
}

static void TaskSequenziali()
{
	pplx::task<std::string> task = pplx::create_task([]() {
		return 42;
	}).then([](int val) {
		return std::to_string(val);
	}).then([](std::string s) {
		std::reverse(s.begin(), s.end());
		return s;
	});

	std::cout << "Il contrario di '42': '" << task.get() << "'\n"; 
}

static void AspettareTaskParalleli()
{
	// senza sleep vengono eseguiti dallo stesso worker thread
	pplx::task<std::thread::id> task1 = pplx::create_task([]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		return std::this_thread::get_id();
	});

	pplx::task<std::thread::id> task2 = pplx::create_task([]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
		return std::this_thread::get_id();
	});

	pplx::task<std::vector<std::thread::id>> task = task1 && task2;

	std::vector<std::thread::id> ids = task.get();
	for (auto id : ids) {
		std::cout << id << '\n';
	}
}

static void AspettareMoltiTaskParalleli()
{
	auto crea_task = [](int sleep_ms) {
		return pplx::create_task([=]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
			return std::this_thread::get_id();
		});
	};

	// alcuni task possono essere eseguiti dallo stesso thread
	std::vector<pplx::task<std::thread::id>> tasks = {
		crea_task(100),
		crea_task(150),
		crea_task(80),
		crea_task(120),
		crea_task(70)
	};

	pplx::task<std::vector<std::thread::id>> task
		= pplx::when_all(std::begin(tasks), std::end(tasks));

	std::vector<std::thread::id> ids = task.get();
	for (auto id : ids) {
		std::cout << id << '\n';
	}
}

static void AspettareIlPrimoTaskCompletato()
{
	// senza sleep vengono eseguiti dallo stesso worker thread
	pplx::task<int> task1 = pplx::create_task([]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::cout << "Completato 1\n";
		return 1;
	});

	pplx::task<int> task2 = pplx::create_task([]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
		std::cout << "Completato 2\n";
		return 2;
	});

	pplx::task<int> task = task1 || task2; // pplx::when_any()
	int res = task.get();
	std::cout << "Ha finito per primo il task con risultato = " << res << '\n';

	(task1 && task2).wait();
}

static void GestireErrori()
{
	pplx::task<void> task = pplx::create_task([]() {
		std::cout << "Si verifica un errore durante l'elaborazione\n";
		throw std::exception("dati errati");
		std::cout << "Fine elaborazione\n";
		return 42;
	}).then([](int res) {
		std::cout << "Risultato=" << res << '\n';
	}).then([](pplx::task<void> antecedent) {
		try	{
			antecedent.get(); // non bloccante
		}
		catch (const std::exception& ex) {
			std::cout << "Esecuzione terminata con errore: " << ex.what() << '\n';
		}
	});

	task.wait();
}

static void CancellareTask()
{
	pplx::cancellation_token_source cts;	
	pplx::task<void> task = pplx::create_task([]() {
		std::cout << "Attendo\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		std::cout << "Fine attesa\n";
	}).then([]() {
		std::cout << "Questo non viene eseguito\n";
	}, cts.get_token());

	cts.cancel();
	//task.wait();
	//return
	
	try {
		task.get();
	}
	catch (const pplx::task_canceled&) {
		std::cout << "Task cancellato\n";
	}
}

static void TaskRipetuto()
{
	int counter = 10;

	auto looped = [&]() {
		return pplx::create_task([&]() {
			counter--;
			std::cout << "counter=" << counter << " tid=" << std::this_thread::get_id() << '\n';
			return counter != 0;
		});
	};

	pplx::task<void> task = pplx::extras::create_iterative_task(looped);
	task.wait();
}

static void TaskCompletatoDaEvento()
{
	pplx::task_completion_event<void> tce;
	pplx::task<void> task = pplx::create_task(tce)
	.then([]() {
		std::cout << "Evento segnalato\n";
	});

	std::cout << "Segnalo evento\n";
	tce.set();
	task.wait();
}

static void Esempio(void(*f)())
{
	f();
	std::cout << "---------------------------------------\n";
}

int main()
{
	Esempio(&FutureNonSiCompongono);
	Esempio(&TaskEsegueAppenaCreato);
	Esempio(&TaskLanciatoAComando);
	Esempio(&TaskRestituisceValore);
	Esempio(&TaskSequenziali);
	Esempio(&AspettareTaskParalleli);
	Esempio(&AspettareMoltiTaskParalleli);
	Esempio(&AspettareIlPrimoTaskCompletato);
	Esempio(&GestireErrori);
	Esempio(&CancellareTask);
	Esempio(&TaskRipetuto);
	Esempio(&TaskCompletatoDaEvento);
	return 0;
}
