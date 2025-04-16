#include <sstream>
#include "ALL.h"
using namespace std;

void VirtualMemoryProcessor::StartPageQueue() {
	if (!(m_PageWaiting->IsEmpty())) {
		unsigned int processID = m_PageWaiting->GetHead()->m_pProcessID;
		unsigned int virtpage = m_PageWaiting->GetHead()->m_virtPage;
		unsigned int realpage = m_PageWaiting->GetHead()->m_realPage;

		Process* process = OperatingSystem::g_pOperatingSystem->GetProcess(processID);
		process->m_AllTime += (GetTime() - process->m_TimeStartWaiting);
		process->m_TimeStartWaiting = GetTime();

		switch (m_PageWaiting->GetHead()->m_type) {
		case (1):
			//Отправляемся выполнять следующее событие в очереди
			PageFault(processID, virtpage);
			m_PageWaiting->RemoveHead();
			Schedule(GetTime() + OperatingSystem::g_pOperatingSystem->m_PageActionTime, this, &VirtualMemoryProcessor::StartPageQueue);
			break;
		case(2):
			//Отправляемся выполнять следующее событие в очереди
			LoadPage(processID, virtpage, realpage);
			m_PageWaiting->RemoveHead();
			Schedule(GetTime() + OperatingSystem::g_pOperatingSystem->m_PageActionTime, this, &VirtualMemoryProcessor::StartPageQueue);
			break;
		case(3):
			//Отправляемся выполнять следующее событие в очереди
			FinishPageLoad(processID, virtpage, realpage);
			m_PageWaiting->RemoveHead();
			Schedule(GetTime() + OperatingSystem::g_pOperatingSystem->m_PageActionTime, this, &VirtualMemoryProcessor::StartPageQueue);
			break;
		default:
			assert(0);
			break;
		}
	}
}

void VirtualMemoryProcessor::PageFault(unsigned int processID, unsigned int virtPage) //Метод, размещающий страницу в ОП (при необходимости также выполняется замещение)
{
	/*std::cout << GetTime() << std::endl;
	for (unsigned int i = 0; i < OperatingSystem::g_pOperatingSystem->GetMainMemory()->GetMainMemorySize(); ++i) {
		std::cout << OperatingSystem::g_pOperatingSystem->GetMainMemory()->GetProcessID(i) << " ";
	}
	std::cout << std::endl;
	for (auto pageiter = OperatingSystem::g_pOperatingSystem->GetStrategy()->GetQueue().GetHead(); pageiter != nullptr; pageiter = pageiter->m_pNext) {
		std::cout << pageiter->m_page << " ";
	}
	std::cout << std::endl;

	OperatingSystem::g_pOperatingSystem->GetStrategy()->GetQueue().PrintQueue();*/

	ostringstream os;
	if (OperatingSystem::g_pOperatingSystem->m_output > 0) {
		os << "PageFault: Process " << processID;
		Log(os.str());
	}
	Process* process = OperatingSystem::g_pOperatingSystem->GetProcess(processID);
	process->m_PageFaults += 1;
	OperatingSystem::g_pOperatingSystem->m_PageFaults += 1;
	// Поиск свободного места для размещения страницы
	int realPage = OperatingSystem::g_pOperatingSystem->GetMainMemory()->PlacePage();
	// нашли свободную страницу?
	if (realPage == -1)
	{
		// не нашли - выберем страницу для замещения
		realPage = OperatingSystem::g_pOperatingSystem->GetStrategy()->GetPageToReplace();
		OperatingSystem::g_pOperatingSystem->GetMainMemory()->SetBitB(realPage, true);
		unsigned int oldProcessID = OperatingSystem::g_pOperatingSystem->GetMainMemory()->GetProcessID(realPage);
		unsigned int oldprocesspage = OperatingSystem::g_pOperatingSystem->GetMainMemory()->GetVirtPage(realPage);
		//Выставим бит присутствия в ОП у старой виртуальной страницы равным 0
		Process* oldProcess = OperatingSystem::g_pOperatingSystem->GetProcess(oldProcessID);
		oldProcess->GetTable()->SetPresence(oldprocesspage, false);
		// Если страницы не было в АС или она была модифицирована
		if ((!(OperatingSystem::g_pOperatingSystem->m_archiveMemory->CheckPage(oldProcessID, oldprocesspage))) || (OperatingSystem::g_pOperatingSystem->GetMainMemory()->GetBitM(realPage) == true))
		{
			process->m_PageSaves += 1;
			OperatingSystem::g_pOperatingSystem->m_PageSaves += 1;
			if (OperatingSystem::g_pOperatingSystem->m_output > 0) {
				os.str("");
				os << "Planning SavePage: Process " << processID;
				Log(os.str());
			}
			//Если очередь ввода/вывода пуста
			if (OperatingSystem::g_pOperatingSystem->GetIODevice()->m_InputOutput->IsEmpty()) {
				//Отправляем процесс в очередь ввода/вывода
				OperatingSystem::g_pOperatingSystem->GetIODevice()->m_InputOutput->AddForIO(processID, oldProcessID, oldprocesspage, virtPage, realPage, 1);
				//Создаём событие
				Schedule(GetTime() + OperatingSystem::g_pOperatingSystem->m_IOTime + OperatingSystem::g_pOperatingSystem->GetIODevice()->GetTimeForOperation(), OperatingSystem::g_pOperatingSystem->GetIODevice(), &IODevice::StartIO);
				process->m_TimeStartWaiting = GetTime();
			}
			else {
				//Отправляем процесс в очередь ввода/вывода
				OperatingSystem::g_pOperatingSystem->GetIODevice()->m_InputOutput->AddForIO(processID, oldProcessID, oldprocesspage, virtPage, realPage, 1);
				process->m_TimeStartWaiting = GetTime();
			}
			return;
		}
	}
	//Выставим бит занятости страничной операцией для найденной реальной страницы
	OperatingSystem::g_pOperatingSystem->GetMainMemory()->SetBitB(realPage, true);

	LoadPage(processID, virtPage, realPage);
}

void VirtualMemoryProcessor::LoadPage(unsigned int processID, unsigned int virtPage, unsigned int realPage) //Метод, проверяющий наличие страницы процесса в АС
{
	ostringstream os;
	Process* process = OperatingSystem::g_pOperatingSystem->GetProcess(processID);

	// есть ли в АС загружаемая страница?
	if (OperatingSystem::g_pOperatingSystem->m_archiveMemory->CheckPage(processID, virtPage))
	{
		if (OperatingSystem::g_pOperatingSystem->m_output > 0) {
			os.str("");
			os << "Planning page load from AM: Process " << processID << " Page: " << virtPage;
			Log(os.str());
		}
		//Если очередь ввода/вывода пуста
		if (OperatingSystem::g_pOperatingSystem->GetIODevice()->m_InputOutput->IsEmpty()) {
			//Отправляем процесс в очередь ввода/вывода
			OperatingSystem::g_pOperatingSystem->GetIODevice()->m_InputOutput->AddForIO(processID, 0, 0, virtPage, realPage, 2);
			//Создаём событие
			Schedule(GetTime() + OperatingSystem::g_pOperatingSystem->m_IOTime + OperatingSystem::g_pOperatingSystem->GetIODevice()->GetTimeForOperation(), OperatingSystem::g_pOperatingSystem->GetIODevice(), &IODevice::StartIO);
			process->m_TimeStartWaiting = GetTime();
		}
		else {
			//Отправляем процесс в очередь ввода/вывода
			OperatingSystem::g_pOperatingSystem->GetIODevice()->m_InputOutput->AddForIO(processID, 0, 0, virtPage, realPage, 2);
			process->m_TimeStartWaiting = GetTime();
		}
		return;
	}
	FinishPageLoad( processID, virtPage, realPage);
}

void VirtualMemoryProcessor::FinishPageLoad(unsigned int processID, unsigned int virtPage, unsigned int realPage) //Метод, завершающий обработку страничной операции
{
	Process* process = OperatingSystem::g_pOperatingSystem->GetProcess(processID);

	//страничная операция завершена
	OperatingSystem::g_pOperatingSystem->GetStrategy()->AddPage(realPage);
	if (OperatingSystem::g_pOperatingSystem->m_output > 0) {
		ostringstream os;
		os.str("");
		os << "FinishPageLoad: Added page " << virtPage << " to strategy queue.";
		Log(os.str());
	}
	//OperatingSystem::g_pOperatingSystem->GetStrategy()->RemovePage(realPage);
	//Заполним биты информации реальной страницы и сохраним новую виртуальную страницу 
	OperatingSystem::g_pOperatingSystem->GetMainMemory()->SetPage(realPage, processID, virtPage);
	OperatingSystem::g_pOperatingSystem->GetMainMemory()->SetBitM(realPage, false);
	OperatingSystem::g_pOperatingSystem->GetMainMemory()->SetBitB(realPage, false);
	OperatingSystem::g_pOperatingSystem->GetMainMemory()->SetBitR(realPage, true);
	OperatingSystem::g_pOperatingSystem->GetMainMemory()->SetBitU(realPage, true);
	OperatingSystem::g_pOperatingSystem->GetMainMemory()->SetRTime(realPage, GetTime());

	if (OperatingSystem::g_pOperatingSystem->m_output > 0) {
		ostringstream os;
		os << "FinishPageLoad: Process " << processID;
		Log(os.str());
	}

	// обновим таблицу страниц процесса
	process->GetTable()->AddRealPage(virtPage, realPage);
	process->GetTable()->SetPresence(virtPage, true);
	//Возвращаем процесс в очередь активных
	//Если активная очередь пустая
	if (OperatingSystem::g_pOperatingSystem->m_Active->IsEmpty()) {
		//Добавляем в активную очередь
		OperatingSystem::g_pOperatingSystem->m_Active->Add(process->GetProcessID());
		//Создаём событие на выполнение процесса
		Schedule(GetTime(), OperatingSystem::g_pOperatingSystem, &OperatingSystem::Start);
		process->m_TimeStartWaiting = GetTime();
	}
	//Если активная очередь не пустая
	else {
		//Добавляем в активную очередь
		OperatingSystem::g_pOperatingSystem->m_Active->Add(process->GetProcessID());
		process->m_TimeStartWaiting = GetTime();
	}
}
