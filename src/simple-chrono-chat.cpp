/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2015 Colorado State University, Fort Collins
 *
 * This file is part of SimpleChronoChat, a simple command chat example based on ChronoSync
 * library, which is a synchronization library for distributed realtime applications for NDN.
 *
 * This file also uses the code for command input from another chronoChat example in NDN-CPP
 * library, which is a NDN client library for C++ and C.
 *
 * SimpleChronoChat is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * SimpleChronoChat is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * SimpleChronoChat, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Chengyu Fan <chengyu@cs.colostate.edu>
 */


#include <poll.h>
#include <iostream>
#include <functional>
#include <memory>
#include <vector>
#include <ChronoSync/socket.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/name-component.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <boost/thread/thread.hpp>

using namespace std;
using namespace ndn;

class Chat {
public:
  Chat(const Name& chatRoomPrefix,
       const Name& userChatPrefix,
       const Name& routingPrefix);

  ~Chat();

  void
  initialize();

  void
  start();

  void
  leave();

  void
  sendMsg(string& msg);

private:
  void
  processSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates);

  void
  displayChatData(const ndn::shared_ptr<const ndn::Data>& data);

private:
  Name m_syncPrefix; // sync sync prefix
  Name m_userChatPrefix; // user chat prefix
  Name m_routingPrefix;  // routable prefix
  Name m_routableUserChatPrefix;

  shared_ptr<ndn::Face> m_face;
  shared_ptr<chronosync::Socket> m_socket; // SyncSocket
};

Chat::Chat(const Name& syncPrefix,
           const Name& userChatPrefix,
           const Name& routingPrefix)
  : m_syncPrefix(syncPrefix)
  , m_userChatPrefix(userChatPrefix)
  , m_routingPrefix(routingPrefix)
{
}

Chat::~Chat()
{
}

void
Chat::initialize()
{
  m_face = make_shared<ndn::Face>();

  // The routable user prefix is the user's name prefix
  m_routableUserChatPrefix.clear();
  m_routableUserChatPrefix.append(m_routingPrefix)
    .append(m_userChatPrefix);

  // 1. To verify the signatures, we need to implement our validator, and create chronosync::Socket
  // 2. The processSyncUpdate is the callback function to receive notification on the state
  // changes, and do what your application needs to do (e.g., figure out what are missing, and
  // how to fetch the new data)
  m_socket = make_shared<chronosync::Socket>(m_syncPrefix,
                                             m_routableUserChatPrefix,
                                             ref(*m_face),
                                             bind(&Chat::processSyncUpdate, this, _1));
}

void
Chat::start()
{
  // processEvents will run the IO service, and continues to process the next event
  m_face->processEvents();
}

void
Chat::leave()
{
  // close the IO service, so the program can exit
  m_face->getIoService().stop();
}

void
Chat::displayChatData(const ndn::shared_ptr<const ndn::Data>& data)
{
  // The data name is /ndn/edu/colostate/susmit/%00%00% ...
  // therefore, the third Component is the user name
  ndn::Name::Component speaker = data->getName().at(3);
  string message(reinterpret_cast<const char*>(data->getContent().value()),
                 data->getContent().value_size());

  cout << speaker.toUri() << " : " <<  message << endl;
}


void
Chat::processSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates)
{
  if (updates.empty()) {
    return;
  }

  for (int i = 0; i < updates.size(); i++) {
    // fetch missing chat data

    if (updates[i].high - updates[i].low < 10) {
      // we just fetch the lastest 10 messages, although the ChronoSync gave all the clues
      for (chronosync::SeqNo seq = updates[i].low; seq <= updates[i].high; ++seq) {
        m_socket->fetchData(updates[i].session, seq,
                            bind(&Chat::displayChatData, this, _1),
                            2);// this 2 means the retry number
      }
    }
    else {
      // There are too many messages, we just fetch the latest 10 of them
      // remember this is for each session
      chronosync::SeqNo startSeq = updates[i].high - 10;
      for (chronosync::SeqNo seq = startSeq; seq <= updates[i].high; ++seq) {
        m_socket->fetchData(updates[i].session, seq,
                            bind(&Chat::displayChatData, this, _1),
                            2);
      }
    }
  }
}

void
Chat::sendMsg(string& msg)
{
  // ChronoSync provide the publishData for us, it does the details for us
  // like update the sequence number, digest etc.
  m_socket->publishData(reinterpret_cast<const uint8_t*>(msg.c_str()), msg.size(),
                        ndn::time::milliseconds(4000));
}

static const char *WHITESPACE_CHARS = " \n\r\t";

/**
 * Modify str in place to erase whitespace on the left.
 * @param str
 */
static inline void
trimLeft(string& str)
{
  size_t found = str.find_first_not_of(WHITESPACE_CHARS);
  if (found != string::npos) {
    if (found > 0)
      str.erase(0, found);
  }
  else
    // All whitespace
    str.clear();
}

/**
 * Modify str in place to erase whitespace on the right.
 * @param str
 */
static inline void
trimRight(string& str)
{
  size_t found = str.find_last_not_of(WHITESPACE_CHARS);
  if (found != string::npos) {
    if (found + 1 < str.size())
      str.erase(found + 1);
  }
  else
    // All whitespace
    str.clear();
}

/**
 * Modify str in place to erase whitespace on the left and right.
 * @param str
 */
static void
trim(string& str)
{
  trimLeft(str);
  trimRight(str);
}

/**
 * Poll stdin and return true if it is ready to ready (e.g. from stdinReadLine).
 */
static bool
isStdinReady()
{
  struct pollfd pollInfo;
  pollInfo.fd = STDIN_FILENO;
  pollInfo.events = POLLIN;

  return poll(&pollInfo, 1, 0) > 0;
}

/**
 * Read a line from from stdin and return a trimmed string.  (We don't use
 * cin because it ignores a blank line.)
 */
static string
stdinReadLine()
{
  char inputBuffer[1000];
  ssize_t nBytes = ::read(STDIN_FILENO, inputBuffer, sizeof(inputBuffer) - 1);
  if (nBytes < 0)
    // Don't expect an error reading from stdin.
    throw runtime_error("stdinReadLine: error reading from STDIN_FILENO");

  inputBuffer[nBytes] = 0;
  string input(inputBuffer);
  trim(input);

  return input;
}

void
input(shared_ptr<Chat>& chat)
{
  chat->start();
}


int main()
{
  try {
    // This sync prefix is for the ChronoSync instances to exchange sync messages
    string sync = "/ndn/broadcast/chronoSyncTest";
    Name t1(sync);

    // The userName and the routingPrefix are used as a whole for users to publish data
    // Assuming a user input "Steve" as its user name, the prefix to publish data should be
    // "/ndn/edu/colostate/Steve"
    cout << "Enter your chat username:" << endl;
    string userName = stdinReadLine();
    Name t2(userName);

    string routingPrefix = "/ndn/edu/colostate";
    Name t3(routingPrefix);

    const char* host = "localhost";
    cout << "Connecting to " << host << ", Chatroom: " << routingPrefix <<
      ", Username: " << userName << endl << endl;

    shared_ptr<Chat> chat(new Chat (t1, t2, t3));

    chat->initialize();

    boost::thread tInput(input, chat);
    cout << "Enter your chat message. To quit, enter \"leave\" or \"exit\"." << endl;

    while (true) { // always wait for the next input, exit when the user input "leave" or "exit"
      if (isStdinReady()){
        string message = stdinReadLine();
        if (message == "leave" || message == "exit") {
          chat->leave();
          break;
        }
        cout << "me : " << message << endl;
        chat->sendMsg(message);
      }
      usleep(100000);
    }
    tInput.join();

  } catch (std::exception& e) {
    cout << "exception: " << e.what() << endl;
  }
  return 0;
}
