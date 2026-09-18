# NREPlatform Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

SRCDIR = src
TARGET = nre

SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/experiment.cpp \
          $(SRCDIR)/tc_control.cpp \
          $(SRCDIR)/logger.cpp \
          $(SRCDIR)/target.cpp \
          $(SRCDIR)/profile.cpp \
          $(SRCDIR)/nft_control.cpp \
          $(SRCDIR)/failure_experiment.cpp \
          $(SRCDIR)/monitor.cpp \
          $(SRCDIR)/policy.cpp \
          $(SRCDIR)/policy_engine.cpp \
          $(SRCDIR)/history.cpp \
          $(SRCDIR)/chain.cpp \
          $(SRCDIR)/resource_guard.cpp \
          $(SRCDIR)/scheduler.cpp \
          $(SRCDIR)/daemon.cpp \
          $(SRCDIR)/api.cpp \
          $(SRCDIR)/audit_log.cpp \
          $(SRCDIR)/permission.cpp \
          $(SRCDIR)/presets.cpp \
          $(SRCDIR)/storage.cpp \
          $(SRCDIR)/test_automation.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET) nre.log
	rm -rf profiles history chains schedules users audit running policies presets data results

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/
	install -m 644 systemd/nre.service /etc/systemd/system/
	install -m 644 systemd/nre-api.service /etc/systemd/system/
	systemctl daemon-reload

uninstall:
	rm -f /usr/local/bin/nre
	rm -f /etc/systemd/system/nre.service
	rm -f /etc/systemd/system/nre-api.service
	systemctl daemon-reload

.PHONY: all clean install uninstall
