#include "ALL.h"

SimulatorTime Processor::GetTimeForRegOperation() {
	return m_timeForReg;
}

SimulatorTime Processor::GetTimeForReadOperation() {
	return m_timeForRead;
}

SimulatorTime Processor::GetTimeForWriteOperation() {
	return m_timeForWrite;
}

unsigned int Processor::UsePageTable(Process* process, unsigned int processID, unsigned int page) {
	if (process->GetTable()->GetPresence(page)) {
		return process->GetTable()->GetRealPage(page);
	}
	else {
		assert(0);
		return 666;
	}
}