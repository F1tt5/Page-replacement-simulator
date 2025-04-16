#include <sstream>
#include "PageDistribution.h"
#include "ALL.h"
using namespace std;


void OperatingSystem::Start() {
	if (!m_Active->IsEmpty()) {
		GetProcess(m_Active->GetHead()->m_pProcessID)->m_AllTime += m_DispatchTime;
		Schedule(GetTime() + m_DispatchTime, this, &OperatingSystem::StartDispatch);
	}
}

void OperatingSystem::StartDispatch() //Выполнение первого в активной очереди процесса или отправление его в очередь ожидания страничной операции
{
	if (!m_Active->IsEmpty()) {
		ostringstream os;
		SimulatorTime t;
		OperationType optype;
		//Если в активной очереди есть процесс(ы)
		//Переменная для хранения нужной процессу страницы
		unsigned int opvirtpage;
		//Переменная для хранения реальной страницы
		unsigned int opRealPage;
		//Переменная хранящая первый элемент активной очереди
		Element* pElement = m_Active->GetHead();
		//Процесс, который хранится в данном элементе
		Process* pProcess = GetProcess(pElement->m_pProcessID);
		//Если у процесса кончилось время процессора
		if (pProcess->GetTimeQuantum() == 0) {
			if (m_output == 2) {
				os.str("");
				os << "End of time: Process " << pProcess->GetProcessID();
				Log(os.str());
			}
			//Убираем его из активной очереди
			m_Active->RemoveHead();
			//Создаём событие на выполнение следующего в очереди процесса
			pProcess->ResetTimeQuantum();
			m_Active->Add(pProcess->GetProcessID());
			Schedule(GetTime(), this, &OperatingSystem::Start);
			return;
		}
		//Выбор операции, которую выполняет процесс
		if (pProcess->GetOperation() == 4) {
			optype = pProcess->ChooseOperation();
			pProcess->SetOperation(optype);
		}
		else {
			optype = pProcess->GetOperation();
		}
		switch (optype)
		{
		case OT_REG:
			if (m_output == 2) {
				os.str("");
				os << "OT_REG: Process " << pProcess->GetProcessID();
				Log(os.str());
			}
			//Если операция в регистре
			//Отправляем команду процессору на выполнение операции в регистре и получаем время выполнения операции
			t = m_processor->GetTimeForRegOperation() * m_Scale;
			pProcess->m_AllTime += t;
			Schedule(GetTime() + t, this, &OperatingSystem::FinishRegDispatch, t, pProcess->GetProcessID());
			break;
		case OT_READ:
			//Если операция чтения
			//Выбираем необходимую процессу страницу
			if (pProcess->GetPage() == -1) {
				opvirtpage = pProcess->ChoosePage();
				pProcess->SetPage(opvirtpage);
			}
			else {
				opvirtpage = pProcess->GetPage();
			}
			if (m_output == 2) {
				os << "OT_READ: Process " << pProcess->GetProcessID() << " Page: " << opvirtpage;
				Log(os.str());
			}
			//Если страница присутствует в ОП
			if (pProcess->GetTable()->GetPresence(opvirtpage)) {
				//Получаем номер реальной страницы процесса
				opRealPage = m_processor->UsePageTable(pProcess, pProcess->GetProcessID(), opvirtpage);
				//Выставляем биты использования и обращения
				m_mainMemory->SetBitU(opRealPage, true);
				m_mainMemory->SetBitR(opRealPage, true);
				m_mainMemory->SetRTime(opRealPage, GetTime());
				//Создаём событие на окончание операции
				t = m_processor->GetTimeForReadOperation() * m_Scale;
				pProcess->m_AllTime += t;
				Schedule(GetTime() + t, this, &OperatingSystem::FinishReadDispatch, pProcess->GetProcessID(), opRealPage);
			}
			//Если не присутствует
			else {
				//Удаляем процесс из активной очереди
				m_Active->RemoveHead();
				//Если очередь страничных операций пустая
				if (m_VMP->m_PageWaiting->IsEmpty()) {
					//Добавляем в очередь ожидающих страничной операции
					m_VMP->m_PageWaiting->AddForPageWaiting(pProcess->GetProcessID(), 1, opvirtpage, 0);
					pProcess->m_TimeStartWaiting = GetTime();
					//Создаём событие на обработку страничной операции
					Schedule(GetTime() + m_PageActionTime, m_VMP, &VirtualMemoryProcessor::StartPageQueue);
				}
				//Если очередь страничных операций не пустая
				else {
					//Добавляем в очередь ожидающих страничной операции
					pProcess->m_TimeStartWaiting = GetTime();
					m_VMP->m_PageWaiting->AddForPageWaiting(pProcess->GetProcessID(), 1, opvirtpage, 0);
				}
				//Создаём событие на выполнение следующего активного процесса
				Schedule(GetTime(), this, &OperatingSystem::Start);
			}
			break;
		case OT_WRITE:
			//Если операция записи
			//Выбираем необходимую процессу страницу
			if (pProcess->GetPage() == -1) {
				opvirtpage = pProcess->ChoosePage();
				pProcess->SetPage(opvirtpage);
			}
			else {
				opvirtpage = pProcess->GetPage();
			}
			if (m_output == 2) {
				os << "OT_WRITE: Process " << pProcess->GetProcessID() << " Page: " << opvirtpage;
				Log(os.str());
			}
			//Если страница присутствует в ОП
			if (pProcess->GetTable()->GetPresence(opvirtpage)) {
				//Получаем номер реальной страницы процесса
				opRealPage = m_processor->UsePageTable(pProcess, pProcess->GetProcessID(), opvirtpage);
				//Выставляем биты использования, обращения и модификации
				m_mainMemory->SetBitU(opRealPage, true);
				m_mainMemory->SetBitR(opRealPage, true);
				m_mainMemory->SetBitM(opRealPage, true);
				m_mainMemory->SetRTime(opRealPage, GetTime());
				t = m_processor->GetTimeForWriteOperation() * m_Scale;
				pProcess->m_AllTime += t;
				//Создаём событие на окончание операции
				Schedule(GetTime() + t, this, &OperatingSystem::FinishWriteDispatch, pProcess->GetProcessID(), opRealPage);
			}
			//Если не присутствует
			else {
				//Удаляем процесс из активной очереди
				m_Active->RemoveHead();
				//Если очередь страничных операций пустая
				if (m_VMP->m_PageWaiting->IsEmpty()) {
					//Добавляем в очередь ожидающих страничной операции
					m_VMP->m_PageWaiting->AddForPageWaiting(pProcess->GetProcessID(), 1, opvirtpage, 0);
					pProcess->m_TimeStartWaiting = GetTime();
					//Создаём событие на обработку страничной операции
					Schedule(GetTime() + m_PageActionTime, m_VMP, &VirtualMemoryProcessor::StartPageQueue);
				}
				//Если очередь страничных операций не пустая
				else {
					//Добавляем в очередь ожидающих страничной операции
					m_VMP->m_PageWaiting->AddForPageWaiting(pProcess->GetProcessID(), 1, opvirtpage, 0);
					pProcess->m_TimeStartWaiting = GetTime();
				}
				//Создаём событие на выполнение следующего активного процесса
				Schedule(GetTime(), this, &OperatingSystem::Start);
			}
			break;
		case OT_IO:
			if (m_output == 2) {
				os << "OT_IO: Process " << pProcess->GetProcessID();
				Log(os.str());
			}
			//Если операция ввода вывода
			//Удаляем процесс из активной очереди и добавляем в очередь ввода/вывода
			m_Active->RemoveHead();
			//Если очередь ввода/вывода пуста
			if (m_iodevice->m_InputOutput->IsEmpty()) {
				//Отправляем процесс в очередь ввода/вывода
				m_iodevice->m_InputOutput->AddForIO(pProcess->GetProcessID(), 0, 0, 0, 0, 3);
				//Создаём событие
				Schedule(GetTime() + m_IOTime + m_iodevice->GetTimeForOperation(), m_iodevice, &IODevice::StartIO);
				pProcess->m_TimeStartWaiting = GetTime();
			}
			else {
				//Отправляем процесс в очередь ввода/вывода
				m_iodevice->m_InputOutput->AddForIO(pProcess->GetProcessID(), 0, 0, 0, 0, 3);
				pProcess->m_TimeStartWaiting = GetTime();
			}
			//Создаём событие на выполнение следующего активного процесса
			Schedule(GetTime(), this, &OperatingSystem::Start);
			break;
		default:
			assert(0);
			break;
		}
	}
}

void OperatingSystem::FinishRegDispatch(SimulatorTime time, unsigned int processID) {
	Process* process = GetProcess(processID);
	if (m_output == 2) {
		ostringstream os;
		os << "OP_PERFORMING - Reg: Process " << processID;
		Log(os.str());
	}
	//Уменьшаем время процесса
	if ((process->GetTimeQuantum() - (m_processor->GetTimeForRegOperation() * m_Scale)) > process->GetReset()) {
		process->DecreaseTimeQuantum(process->GetTimeQuantum());
	}
	else {
		process->DecreaseTimeQuantum(m_processor->GetTimeForRegOperation() * m_Scale);
	}
	process->m_operationsDone[0] += 1 * m_Scale;
	process->m_WorkTime += static_cast<SimulatorTime>(m_processor->GetTimeForRegOperation() * m_Scale);
	process->SetOperation(OT_ZERO);
	Schedule(GetTime(), this, &OperatingSystem::StartDispatch);
}

void OperatingSystem::FinishReadDispatch(unsigned int processID, unsigned int realPage) {
	Process* process = GetProcess(processID);
	if (m_output == 2) {
		ostringstream os;
		os << "OP_FINISH - Read: Process " << process->GetProcessID();
		Log(os.str());
	}
	//Уменьшаем время процесса
	if ((process->GetTimeQuantum() - (m_processor->GetTimeForReadOperation()* m_Scale)) > process->GetReset()) {
		process->DecreaseTimeQuantum(process->GetTimeQuantum());
	}
	else {
		process->DecreaseTimeQuantum(m_processor->GetTimeForReadOperation()* m_Scale);
	}
	//Выставляем бит использования страницы
	m_mainMemory->SetBitU(realPage, false);
	process->m_operationsDone[1] += 1 * m_Scale;
	process->m_WorkTime += static_cast<SimulatorTime>(m_processor->GetTimeForReadOperation() * m_Scale);
	process->SetOperation(OT_ZERO);
	process->SetPage(-1);
	Schedule(GetTime(), this, &OperatingSystem::StartDispatch);
}

void OperatingSystem::FinishWriteDispatch(unsigned int processID, unsigned int realPage) {
	Process* process = GetProcess(processID);
	if (m_output == 2) {
		ostringstream os;
		os << "OP_FINISH - Write: Process " << process->GetProcessID();
		Log(os.str());
	}
	//Уменьшаем время процесса
	if ((process->GetTimeQuantum() - (m_processor->GetTimeForWriteOperation()* m_Scale)) > process->GetReset()) {
		process->DecreaseTimeQuantum(process->GetTimeQuantum());
	}
	else {
		process->DecreaseTimeQuantum(m_processor->GetTimeForWriteOperation()* m_Scale);
	}
	m_mainMemory->SetBitU(realPage, false);
	process->m_operationsDone[2] += 1 * m_Scale;
	process->m_WorkTime += static_cast<SimulatorTime>(m_processor->GetTimeForWriteOperation() * m_Scale);
	process->SetOperation(OT_ZERO);
	process->SetPage(-1);
	Schedule(GetTime(), this, &OperatingSystem::StartDispatch);

}

void OperatingSystem::FinishIODispatch(unsigned int processID) {
	Process* process = GetProcess(processID);
	if (m_output == 2) {
		ostringstream os;
		os << "OP_FINISH - IO: Process " << process->GetProcessID();
		Log(os.str());
	}
	//Уменьшаем время процесса
	if ((process->GetTimeQuantum() - m_iodevice->GetTimeForOperation()) > process->GetReset()) {
		process->DecreaseTimeQuantum(process->GetTimeQuantum());
	}
	else {
		process->DecreaseTimeQuantum(m_iodevice->GetTimeForOperation());
	}
	process->m_operationsDone[3] += 1;
	//process->m_WorkTime += static_cast<SimulatorTime>(m_iodevice->GetTimeForOperation());
	process->SetOperation(OT_ZERO);
	//Если очередь активных процессов пуста
	if (m_Active->IsEmpty()) {
		//Добавляем туда наш процесс
		m_Active->Add(process->GetProcessID());
		process->m_TimeStartWaiting = GetTime();
		//Создаём событие на выполнение нашего процесса
		Schedule(GetTime(), this, &OperatingSystem::Start);
	}
	//Если есть активные процессы
	else {
		//Добавляем в очередь активных наш процесс
		m_Active->Add(process->GetProcessID());
		process->m_TimeStartWaiting = GetTime();
	}
}

void OperatingSystem::ChooseStrategy(unsigned int strategyNumber) {
	//Создание массива из объектов классов стратегий замещения
	PageReplacementStrategy* array[] = { new FIFO(),
										 new Random(),
										 new SecondChance(),
										 new Clock(),
										 new NRU(),
										 new Aging(),
										 new LRU()
	};
	//Получение размера массива
	unsigned int n = sizeof(array) / sizeof(array[0]);
	
	//Если введён один из номеров стратегий
	if (strategyNumber >= 0 && strategyNumber < n) {
		//Создаём объект стратегии выбранного типа
		PageReplacementStrategy* selectedStrategy = array[strategyNumber];
		//Делаем ссылку на эту стратегию в ОС
		m_Strategy = selectedStrategy;
		m_Strategy->SetMM(m_mainMemory);
	}
	//Если введён неверный номер
	else {
		std::cout << "Wrong strategy number";
	}
	// Освобождаем память
	for (unsigned int i = 0; i < n; ++i) {
		if (i != strategyNumber) {
			delete array[i];
		}
	}
}

float* OperatingSystem::ChooseDistribution(unsigned int processPageSize, unsigned int distributionID, float parameter) {
    // Создаём список из типов распределения страниц
    PageDistribution* array[] = {new Uniform,
                                 new Exponential,
                                 new Pareto
    };
    // Находим размер списка
    unsigned int n = sizeof(array) / sizeof(array[0]);
    float* distribution = nullptr;

    // Если номер верный
    if (distributionID > 0 && distributionID <= n) {  
        // Создаём распределение выбранного типа 
        distribution = array[distributionID]->CreateDistribution(processPageSize, parameter);
    }
    // Если неверный
    else {
        std::cout << "Wrong distribution number";
    }

    // Освобождаем память от всех созданных объектов распределений
    for (unsigned int i = 0; i < n; ++i) {
        delete array[i];
    }

    return distribution;
}

void OperatingSystem::StartProcess(unsigned int processSystemID) {
	if (m_Active->IsEmpty()) {
		//Добавляем туда наш процесс
		m_Active->Add(m_processes[processSystemID]->GetProcessID());
		//Создаём событие на выполнение нашего процесса
		m_processes[processSystemID]->m_nextEvent = Schedule(GetTime() + m_DispatchTime, this, &OperatingSystem::Start);
	}
	//Если есть активные процессы
	else {
		//Добавляем в очередь активных наш процесс
		m_Active->Add(m_processes[processSystemID]->GetProcessID());
	}
}

void OperatingSystem::RemoveProcess(unsigned int processID) {
	Process* process = GetProcess(processID);

	if (process->m_nextEvent != 0) {
		CancelEvent(process->m_nextEvent);
	}

	m_Active->Remove(processID);
	m_VMP->m_PageWaiting->Remove(processID);
	m_iodevice->m_InputOutput->Remove(processID);

	m_mainMemory->DeleteProcess(processID);
	
	for (unsigned int i = 0; i < process->GetSize(); ++i) {
		process->m_processPageComeBacksCount[i] += m_archiveMemory->GetReturns(processID, i);
	}
	m_archiveMemory->DeleteProcess(processID);
}

Process* OperatingSystem::GetProcess(unsigned int processID) {
	Process* process = NULL;
	for (unsigned int i = 0; i < m_processesSize; ++i) {
		if (m_processes[i]->GetProcessID() == processID) {
			process = m_processes[i];
			break;
		}
	}
	if (process == NULL) {
		assert(0);
	}
	return process;
}

void OperatingSystem::ResetBitR() {
	for (unsigned int i = 0; i < m_mainMemory->GetMainMemorySize(); ++i) {
		m_mainMemory->SetBitR(i, false);
	}
	Schedule(GetTime() + m_Strategy->GetResetTime(), this, &OperatingSystem::ResetBitR);
}

void OperatingSystem::UpdateAgeCounters() {
	m_Strategy->UpdateAgeCounters();
	// Планируем следующее обновление через определённый интервал времени
	Schedule(GetTime() + m_Strategy->GetResetTime(), this, &OperatingSystem::UpdateAgeCounters);
}

void OperatingSystem::CollectInfo(unsigned int iter, SimulatorTime time, unsigned int number) {
	SimulatorTime WorkTime = 0;
	for (unsigned int i = 0; i < m_processesSize; ++i) {
		WorkTime += m_processes[i]->m_WorkTime;
	}

	float OverheadExpenses = (1.0f - (static_cast<float>(WorkTime) / (g_pSim->GetTime() - 1.0f))) * 100.0f;
	unsigned int OPspace = m_mainMemory->GetFreeSpace();
	unsigned int AMspace = m_archiveMemory->GetFreeSpace();

	m_InfoTable[iter].m_time = time * static_cast<SimulatorTime>(iter + 1);
	m_InfoTable[iter].m_OverheadExpenses = OverheadExpenses;
	m_InfoTable[iter].m_OPspace = OPspace;
	m_InfoTable[iter].m_AMspace = AMspace;
	if (iter < number) {
		Schedule(GetTime() + time, this, &OperatingSystem::CollectInfo, iter + 1, time, number);
	}
}
