#include "ALL.h"

int MainMemory::PlacePage() //Метод, находящий свободное место в ОП.
{
	int page = -1;
	// ищем свободную страницу
	for (unsigned int i = 0; i < m_npages; ++i)
	{
		if ((m_pages[i].m_number == -1) && (m_pages[i].m_processID == -1) && (m_pages[i].m_busy == false))
		{
			page = i;
			break;
		}
	}
	return page;
}

void MainMemory::DeleteProcess(unsigned int processID) { //Метод, удаляющий страницы процесса из ОП.
	// Проход по списку страниц в ОП
	for (unsigned int i = 0; i < m_npages; ++i) {
		//Если страница принадлежит введённому процессу
		if (m_pages[i].m_processID == processID) {
			//Очищение информации о битах
			m_pages[i].m_processID = -1;
			m_pages[i].m_number = -1;
			m_pages[i].m_used = false;
			m_pages[i].m_referenced = false;
			m_pages[i].m_modified = false;
			m_pages[i].m_busy = false;
			OperatingSystem::g_pOperatingSystem->GetStrategy()->RemovePage(i);
		}
	}
}

bool MainMemory::CheckPage(unsigned int processID, unsigned int page) { //Метод, проверяющий наличие страницы в ОП
	for (unsigned int i = 0; i < m_npages; ++i) {
		if ((m_pages[i].m_processID == processID) && (m_pages[i].m_number == page)) {
			return true;
		}
	}
	return false;
}

unsigned int MainMemory::GetFreeSpace() { //Метод, проверяющий наличие страницы в ОП
	unsigned int j = 0;
	for (unsigned int i = 0; i < m_npages; ++i) {
		if ((m_pages[i].m_number == -1) && (m_pages[i].m_busy == false)) {
			++j;
		}
	}
	return j;
}
