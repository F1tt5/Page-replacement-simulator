#include <sstream>
#include "ALL.h"
using namespace std;

void IODevice::StartIO() {
	if (!(m_InputOutput->IsEmpty())) {
		ostringstream os;
		unsigned int processID = m_InputOutput->GetHead()->m_pProcessID;
		unsigned int virtpage = m_InputOutput->GetHead()->m_virtPage;
		unsigned int realpage = m_InputOutput->GetHead()->m_realPage;
		unsigned int oldprocessID = m_InputOutput->GetHead()->m_pOldProcessID;
		unsigned int oldprocesspage = m_InputOutput->GetHead()->m_oldProcessPage;

		Process* process = OperatingSystem::g_pOperatingSystem->GetProcess(processID);
		process->m_AllTime += (GetTime() - process->m_TimeStartWaiting);

		switch (m_InputOutput->GetHead()->m_type) {
		case (1):
			//Добавим страницу в АС
			OperatingSystem::g_pOperatingSystem->m_archiveMemory->SavePage(oldprocessID, oldprocesspage);
			//Переходим к следующей операции
			if (OperatingSystem::g_pOperatingSystem->m_output > 0) {
				os << "SavePage: Process " << oldprocessID << " Page: " << oldprocesspage;
				Log(os.str());
			}
			m_InputOutput->RemoveHead();
			if (OperatingSystem::g_pOperatingSystem->GetVMP()->m_PageWaiting->IsEmpty()) {
				//Отправляем процесс в очередь страничных операций
				OperatingSystem::g_pOperatingSystem->GetVMP()->m_PageWaiting->AddForPageWaiting(processID, 2, virtpage, realpage);
				//Создаём событие
				Schedule(GetTime() + OperatingSystem::g_pOperatingSystem->m_PageActionTime, OperatingSystem::g_pOperatingSystem->GetVMP(), &VirtualMemoryProcessor::StartPageQueue);
				process->m_TimeStartWaiting = GetTime();
			}
			else {
				//Отправляем процесс в очередь страничных операций
				OperatingSystem::g_pOperatingSystem->GetVMP()->m_PageWaiting->AddForPageWaiting(processID, 2, virtpage, realpage);
				process->m_TimeStartWaiting = GetTime();
			}
			//Отправляемся выполнять следующее событие в очереди
			Schedule(GetTime() + OperatingSystem::g_pOperatingSystem->m_IOTime + GetTimeForOperation(), this, &IODevice::StartIO);
			break;
		case(2):
			//Загружаем страницу из АС
			OperatingSystem::g_pOperatingSystem->m_archiveMemory->LoadPageFromArch(processID, virtpage);
			//Переходим к следующей операции
			if (OperatingSystem::g_pOperatingSystem->m_output > 0) {
				os << "LoadPageFromArch: Process " << processID << " Page: " << virtpage;
				Log(os.str());
			}
			m_InputOutput->RemoveHead();
			if (OperatingSystem::g_pOperatingSystem->GetVMP()->m_PageWaiting->IsEmpty()) {
				//Отправляем процесс в очередь страничных операций
				OperatingSystem::g_pOperatingSystem->GetVMP()->m_PageWaiting->AddForPageWaiting(processID, 3, virtpage, realpage);
				//Создаём событие
				Schedule(GetTime() + OperatingSystem::g_pOperatingSystem->m_PageActionTime, OperatingSystem::g_pOperatingSystem->GetVMP(), &VirtualMemoryProcessor::StartPageQueue);
				process->m_TimeStartWaiting = GetTime();
			}
			else {
				//Отправляем процесс в очередь страничных операций
				OperatingSystem::g_pOperatingSystem->GetVMP()->m_PageWaiting->AddForPageWaiting(processID, 3, virtpage, realpage);
				process->m_TimeStartWaiting = GetTime();
			}
			//Отправляемся выполнять следующее событие в очереди
			Schedule(GetTime() + OperatingSystem::g_pOperatingSystem->m_IOTime + GetTimeForOperation(), this, &IODevice::StartIO);
			break;
		case(3):
			//Создаём событие на завершение операции ввода/вывода
			process->m_nextEvent = Schedule(GetTime(), OperatingSystem::g_pOperatingSystem, &OperatingSystem::FinishIODispatch, processID);
			//Переходим к следующей операции
			m_InputOutput->RemoveHead();
			//Отправляемся выполнять следующее событие в очереди
			Schedule(GetTime() + OperatingSystem::g_pOperatingSystem->m_IOTime + GetTimeForOperation(), this, &IODevice::StartIO);
			break;
		default:
			assert(0);
			break;
		}
	}
}
