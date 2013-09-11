//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2011
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################
/*
 * XpadClient.cpp
 * Created on: August 01, 2013
 * Author: Hector PEREZ PONCE
 */

#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <cmath>
#include <cstring>

#include <stdarg.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>
#include <stdlib.h>

#include "imXpadClient.h"
#include "ThreadUtils.h"
#include "Exceptions.h"
#include "Debug.h"

//static char errorMessage[1024];

const int MAX_ERRMSG = 1024;
const int CR = '\15';				// carriage return
const int LF = '\12';				// line feed
const char QUIT[] = "quit\n";		// sent using 'send'

using namespace std;
using namespace lima;
using namespace lima::imXpad;

XpadClient::XpadClient() : m_debugMessages() {
    DEB_CONSTRUCTOR();
    // Ignore the sigpipe we get we try to send quit to
    // dead server in disconnect, just use error codes
    struct sigaction pipe_act;
    sigemptyset(&pipe_act.sa_mask);
    pipe_act.sa_flags = 0;
    pipe_act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &pipe_act, 0);
    m_valid = 0;
}

XpadClient::~XpadClient() {
    DEB_DESTRUCTOR();
}

void XpadClient::sendWait(string cmd) {
    cout << "Inside sendWait" << endl;
    DEB_MEMBER_FUNCT();
    int rc;
    DEB_TRACE() << "sendWait(" << cmd << ")";
    AutoMutex aLock(m_cond.mutex());
    if (waitForPrompt() != 0) {
        disconnectFromServer();
        THROW_HW_ERROR(Error) << "Time-out before client sent a prompt. Disconnecting.\n";
    }
    sendCmd(cmd);
    if (waitForResponse(rc) < 0) {
        THROW_HW_ERROR(Error) << "Waiting for response from server";
    }
    if (rc < 0) {
        THROW_HW_ERROR(Error) << "[ " << m_errorMessage << " ]";
    }
}

void XpadClient::sendWait(string cmd, int& value) {
    cout << "Inside sendWait" << endl;
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "sendWait(" << cmd << ")";
    AutoMutex aLock(m_cond.mutex());
    if (waitForPrompt() != 0) {
        disconnectFromServer();
        THROW_HW_ERROR(Error) << "Time-out before client sent a prompt. Disconnecting.\n";
    }
    sendCmd(cmd);
    if (waitForResponse(value) < 0) {
        THROW_HW_ERROR(Error) << "! Waiting for response from server";
    }
    if (value < 0) {
        THROW_HW_ERROR(Error) << "[ " << m_errorMessage << " ]";
    }
}

void XpadClient::sendWait(string cmd, double& value) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "sendWait(" << cmd << ")";
    AutoMutex aLock(m_cond.mutex());
    if (waitForPrompt() != 0) {
        disconnectFromServer();
        THROW_HW_ERROR(Error) << "Time-out before client sent a prompt. Disconnecting.\n";
    }
    sendCmd(cmd);
    if (waitForResponse(value) < 0) {
        THROW_HW_ERROR(Error) << "Waiting for response from server";
    }
    if (isnan(value)) {
        THROW_HW_ERROR(Error) << "[ " << m_errorMessage << " ]";
    }
}

void XpadClient::sendWait(string cmd, string& value) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "sendWait(" << cmd << ")";
    AutoMutex aLock(m_cond.mutex());
    if (waitForPrompt() != 0) {
        disconnectFromServer();
        THROW_HW_ERROR(Error) << "Time-out before client sent a prompt. Disconnecting.\n";
    }
    sendCmd(cmd);
    if (waitForResponse(value) < 0) {
        THROW_HW_ERROR(Error) << "Waiting for response from server";
    }
    if (value.compare("(null)") == 0) {
        THROW_HW_ERROR(Error) << "[ " << m_errorMessage << " ]";
    }
}


void XpadClient::sendNowait(string cmd) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "sendNowait(" << cmd << ")";
    AutoMutex aLock(m_cond.mutex());
    if (waitForPrompt() != 0) {
        disconnectFromServer();
        THROW_HW_ERROR(Error) << "Time-out before client sent a prompt. Disconnecting.\n";
    }
    sendCmd(cmd);
}

void XpadClient::getData(void *bptr, int num, unsigned int xpad_format ) {
    DEB_MEMBER_FUNCT();
    int rc = 0;			// never assigned???? (seb)
    int dataPort;
    unsigned short value_short;
    unsigned int value_int;
    unsigned short *buffer_short;
    unsigned int *buffer_int;


    int IMG_LINE = 120;
    int IMG_NUMBER = num * 80;
    int readsize = IMG_LINE * IMG_NUMBER;

    if (xpad_format==0)
        buffer_short = (unsigned short *)bptr;

    else if (xpad_format==1)
        buffer_int = (unsigned int *)bptr;

    // Allow the server to connect to our data port.
    struct sockaddr_in addr;
    int size = sizeof(struct sockaddr_in);

    dataPort = accept(m_data_listen_skt, (struct sockaddr *) &addr, (socklen_t*)&size);

    if (dataPort < 0) {
        THROW_HW_ERROR(Error) << "Server could not to connect to our data port";
    }

    for (int i=0 ; i<readsize; i++){
        if (xpad_format==0){
            read(dataPort, &value_short, sizeof(unsigned short));
            buffer_short[i]=value_short;
        }
        else if (xpad_format==1){
            read(dataPort, &value_int, sizeof(unsigned int));
            buffer_int[i]=value_int;
        }
    }

    close(dataPort);
    if (rc < 0) {
        THROW_HW_ERROR(Error) << "Read error from data port " << dataPort;
    }
}

/*
 * Connect to remote server
 */
int XpadClient::connectToServer(const string hostname, int port) {
    DEB_MEMBER_FUNCT();
    struct hostent *host;
    struct protoent *protocol;
    int opt;
    int rc = 0;

    if (m_valid) {
        m_errorMessage = "Already connected to server";
        return -1;
    }
    if ((host = gethostbyname(hostname.c_str())) == 0) {
        m_errorMessage = "can't get gethostbyname";
        endhostent();
        return -1;
    }
    if ((m_skt = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        m_errorMessage = "can't create socket";
        endhostent();
        return -1;
    }
    m_remote_addr.sin_family = host->h_addrtype;
    m_remote_addr.sin_port = htons (port);
    size_t len = host->h_length;
    memcpy(&m_remote_addr.sin_addr.s_addr, host->h_addr, len);
    endhostent();
    if (connect(m_skt, (struct sockaddr *) &m_remote_addr, sizeof(struct sockaddr_in)) == -1) {
        close(m_skt);
        m_errorMessage = "Connection to server refused. Is the server running?";
        return -1;
    }
    protocol = getprotobyname("tcp");
    if (protocol == 0) {
        m_errorMessage = "Cannot get protocol TCP\n";
        rc = -1;
    } else {
        opt = 1;
        if (setsockopt(m_skt, protocol->p_proto, TCP_NODELAY, (char *) &opt, 4) < 0) {
            m_errorMessage = "Cannot Set socket options";
            rc = -1;
        }
    }
    endprotoent();
    m_valid = 1;
    m_data_port = -1;
    m_data_listen_skt = -1;
    m_prompts = 0;
    m_num_read = 0;
    m_cur_pos = 0;
    return rc;
}

void XpadClient::disconnectFromServer() {
    DEB_MEMBER_FUNCT();
    if (m_valid) {
        send(m_skt, QUIT, strlen(QUIT), 0);
        shutdown(m_skt, 2);
        close(m_skt);
        m_valid = 0;
    }
}

int XpadClient::initServerDataPort() {
    cout << "Inside initServerDataPort" << endl;
    DEB_MEMBER_FUNCT();
    struct sockaddr_in data_addr;
    int len = sizeof(struct sockaddr_in);
    int one = 0x7FFFFFFF;

    if (m_data_port == -1) {
        if ((m_data_listen_skt = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            m_errorMessage = "can't create data socket";
            return -1;
        }
        if (setsockopt(m_data_listen_skt, SOL_SOCKET, SO_REUSEADDR, (char *) &one, 4) < 0) {
            m_errorMessage = "can't set socket options";
            return -1;
        }
        // Bind the listening socket so that the server may connect to it.
        // Create a unique port number for the server to connect to.
        // Assign a port number at random. Allow anyone to connect.
        data_addr.sin_family = AF_INET;
        data_addr.sin_addr.s_addr = INADDR_ANY;
        data_addr.sin_port = 0;
        if (bind(m_data_listen_skt, (struct sockaddr *) &data_addr, sizeof(struct sockaddr_in)) == -1) {
            m_errorMessage = "can't bind to socket";
            return -1;
        }
        if (listen(m_data_listen_skt, 1) == -1) {
            m_errorMessage = "error in listen";
            close(m_data_listen_skt);
            return -1;
        }
        /* Find out which port was used */
        if (getsockname(m_data_listen_skt, (struct sockaddr *) &data_addr, (socklen_t*)&len) == -1) {
            m_errorMessage = "can't get socket name";
            close(m_data_listen_skt);
            return -1;
        }
        m_data_port = ntohs (data_addr.sin_port);
        if (waitForPrompt() == -1)
            return -1;
        stringstream ss;
        ss << "Port " << m_data_port;
        sendCmd(ss.str());
        int null = 0;
        if (waitForResponse(null) == -1)
            return -1;
    }
    cout << "Getting out of initServerDataPort" << endl;
    return m_data_port;
}

string XpadClient::getErrorMessage() const {
    return m_errorMessage;
}

vector<string> XpadClient::getDebugMessages() const {
    return m_debugMessages;
}

void XpadClient::sendCmd(const string cmd) {
    cout << "Inside sendCmd" << endl;
    cout << "--- cmd: " << cmd << endl;
    DEB_MEMBER_FUNCT();
    int r, len;
    char *p;
    string command = cmd + "\n";
    if (!m_valid) {
        THROW_HW_ERROR(Error) << "Not connected to server ";
    }
    len = command.length();
    p = (char*) command.c_str();
    while (len > 0 && (r = send(m_skt, p, len, 0)) > 0)
        p += r, len -= r;

    if (r <= 0) {
        THROW_HW_ERROR(Error) << "Sending command " << cmd << " to server failed";
    }
}

int XpadClient::waitForPrompt() {
    cout << "Inside waitForPrompt" << endl;
    DEB_MEMBER_FUNCT();
    int r;
    char tmp;

    if (!m_valid) {
        THROW_HW_ERROR(Error) << "Not connected to server ";
    }
    DEB_TRACE() << "checking connection with recv";
    r = recv(m_skt, &tmp, 1, MSG_PEEK | MSG_DONTWAIT);
    if (r == 0 || (r < 0 && errno != EWOULDBLOCK)) {
        DEB_TRACE() << "waitForPrompt: Connection broken, r= " << r << " errno=" << errno;
        return -1;
    }
    if (m_prompts == 0) {
        do {
            r = nextLine(0, 0, 0, 0, 0, 0);
        } while (r != CLN_NEXT_PROMPT);
    } else {
        m_prompts--;
    }
    cout << "Getting out of waitForPrompt" << endl;
    return 0;
}

/*
 *  Wait for an integer response
 */
int XpadClient::waitForResponse(int& value) {
    cout << "Inside waitForResponse" << endl;
    DEB_MEMBER_FUNCT();
    int r, code, done, outoff;
    string errmsg;
    m_debugMessages.clear();

    if (!m_valid) {
        THROW_HW_ERROR(Error) << "Not connected to server ";
    }
    for (;;) {
        r = nextLine(&errmsg, &code, 0, 0, &done, &outoff);
        switch (r) {
        case CLN_NEXT_PROMPT:
            error_handler("(warning) No return code from the server.");
            m_prompts++;
            return -1;
        case CLN_NEXT_ERRMSG:
            errmsg_handler(errmsg);
            break;
        case CLN_NEXT_DEBUGMSG:
            debugmsg_handler(errmsg);
            break;
        case CLN_NEXT_UNKNOWN:
            error_handler("Unknown string from server: "+ errmsg);
            break;
        case CLN_NEXT_TIMEBAR:
            timebar_handler(done, outoff, errmsg);
            break;
        case CLN_NEXT_INTRET:
            value = code;
            cout << "--- Value: " << value << endl;
            return 0;
        case CLN_NEXT_DBLRET:
            error_handler("Unexpected double return code.");
            return -1;
        case CLN_NEXT_STRRET:
            error_handler("Server responded with a string.");
            return -1;
        default:
            THROW_HW_ERROR(Error) << "Programming error. Please report";
        }
    }
    cout << "Getting out of waitForResponse" << endl;
    return 0;
}

/*
 *  Wait for an double response
 */
int XpadClient::waitForResponse(double& value) {
    DEB_MEMBER_FUNCT();
    int r, done, outoff;
    double code;
    string errmsg;
    m_debugMessages.clear();

    if (!m_valid) {
        THROW_HW_ERROR(Error) << "Not connected to server ";
    }
    for (;;) {
        r = nextLine(&errmsg, 0, &code, 0, &done, &outoff);
        switch (r) {
        case CLN_NEXT_PROMPT:
            error_handler("(warning) No return code from the server.");
            m_prompts++;
            return -1;
        case CLN_NEXT_ERRMSG:
            errmsg_handler(errmsg);
            break;
        case CLN_NEXT_DEBUGMSG:
            debugmsg_handler(errmsg);
            break;
        case CLN_NEXT_UNKNOWN:
            error_handler("Unknown string from server: "+ errmsg);
            break;
        case CLN_NEXT_TIMEBAR:
            timebar_handler(done, outoff, errmsg);
            break;
        case CLN_NEXT_INTRET:
            error_handler("Unexpected integer return code.");
            return -1;
        case CLN_NEXT_DBLRET:
            value = code;
            return 0;
        case CLN_NEXT_STRRET:
            error_handler("Server responded with a string.");
            return -1;
        default:
            THROW_HW_ERROR(Error) << "Programming error. Please report";
        }
    }
    return 0;
}

/*
 *  Waits for a string response
 */
int XpadClient::waitForResponse(string& value) {
    DEB_MEMBER_FUNCT();
    int r, done, outoff;
    string errmsg;
    m_debugMessages.clear();

    if (!m_valid) {
        THROW_HW_ERROR(Error) << "Not connected to server ";
    }
    for (;;) {
        r = nextLine(&errmsg, 0, 0, &value, &done, &outoff);
        switch (r) {
        case CLN_NEXT_PROMPT:
            error_handler("(warning) No return code from the server.");
            m_prompts++;
            return -1;
        case CLN_NEXT_ERRMSG:
            errmsg_handler(errmsg);
            break;
        case CLN_NEXT_DEBUGMSG:
            debugmsg_handler(errmsg);
            break;
        case CLN_NEXT_UNKNOWN:
            error_handler("Unknown string from server.");
            break;
        case CLN_NEXT_TIMEBAR:
            timebar_handler(done, outoff, errmsg);
            break;
        case CLN_NEXT_INTRET:
            error_handler("Server responded with an integer");
            return -1;
        case CLN_NEXT_DBLRET:
            error_handler("Server responded with a double");
            return -1;
        case CLN_NEXT_STRRET:
            cout << "--- Message: " << value << endl;
            return (value.length() == 0) ? -1 : 0;
        default:
            THROW_HW_ERROR(Error) << "Programming error. Please report";
        }
    }
    return 0;
}

/*
 * Get the next line from the server
 */
int XpadClient::nextLine(string *errmsg, int *ivalue, double *dvalue, string *svalue, int *done, int *outoff) {
    cout << "Inside nextline " << endl;
    DEB_MEMBER_FUNCT();
    int r;
    char timestr[80], *tp;
    char buff[MAX_ERRMSG + 2];
    char *bptr = buff;
    char *bptr2 = buff;
    int n, len;
    // Assume we are either at the beginning of a line now, or we are right
    // at the end of the previous line.
    do {
        r = getChar();
    } while (r == CR || r == LF);


    switch (r) {
    case -1:						// read error (disconnected?)
        THROW_HW_ERROR(Error) << "server read error (disconnected?)";

    case '>':						// at prompt
        getChar();					// discard ' '
        return CLN_NEXT_PROMPT;

    case '!':						// error message
        getChar();					// discard ' '
        while ((r = getChar()) != -1 && r != CR && r != LF) {
            *bptr++ = r;
        }
        *bptr = '\0';
        if(errmsg != 0)
            *errmsg = buff;
        return CLN_NEXT_ERRMSG;

    case '#':						// comment
        getChar();					// discard ' '
        while ((r = getChar()) != -1 && r != CR && r != LF) {
            *bptr++ = r;
        }
        *bptr = '\0';
        if(errmsg != 0)
            *errmsg = buff;
        return CLN_NEXT_DEBUGMSG;

    case '@':						// timebar message ("a/b")
        getChar();					// discard ' '
        tp = timestr;				// read time message
        for (;;) {
            r = getChar();
            if (r == '\'' || r == '"' || r == CR || r == LF || r == -1)
                break;
            *tp++ = r;
        }
        *tp = '\0';
        if (done != 0 && outoff != 0)
            sscanf(timestr, "%d %d", done, outoff);
        if (r == '\'' || r == '"') {
            while ((r = getChar()) != -1 && r != CR && r != LF) {
                *bptr++ = r;
            }
        } else {
            *bptr = '\0';
        }
        *bptr = '\0';
        len = strlen(buff);
        if (len > 0 && (bptr2[len - 1] == '\'' || bptr2[len - 1] == '"')) {
            bptr2[len - 1] = 0;
        }
        return CLN_NEXT_TIMEBAR;

    case '*': 						// return value
        getChar(); 					// discard ' '
        r = getChar();				// distinguish strings/nums
        if (r == '(')				// (null)
        {

            while ((r = getChar()) != -1 && r != ')' && r != CR && r != LF)
                ;
            if (svalue != 0)
                *svalue = "";
            return CLN_NEXT_STRRET;
        } else if (r == '"') /*		string */
        {
            while ((r = getChar()) != -1 && r != '"') {
                *bptr++ = r;
            }
            *bptr = '\0';
            if (svalue != 0)
                *svalue = buff;
            return CLN_NEXT_STRRET;
        } else {  // integer or double
            char buffer[80], *p;

            buffer[0] = r;
            p = &buffer[1];

            while ((r = getChar()) != -1 && r != CR && r != LF)
                *p++ = r;
            *p = '\0';

            if (dvalue) {
                if (strcmp(buffer, "nan") == 0) {
                    *dvalue = NAN;
                } else {
                    *dvalue = atof(buffer);
                }
                return CLN_NEXT_DBLRET;
            }
            if (ivalue) {
                *ivalue = atoi(buffer);
            }
            return CLN_NEXT_INTRET;
        }

    default: // don't know: abort to end of line
        n = 0;
        while ((r = getChar()) != -1 && r != CR && r != LF) {
            if (++n == MAX_ERRMSG)
                break;
            *bptr++ = r;
        }
        bptr += '\0';
        if (errmsg != 0)
            *errmsg = buff;
        return CLN_NEXT_UNKNOWN;
    }
}

int XpadClient::getChar() {
    DEB_MEMBER_FUNCT();
    int r;

    if (!m_valid) {
        THROW_HW_ERROR(Error) << "Not connected to xpad server ";
    }
    if (m_num_read == m_cur_pos) {
        while ((r = recv(m_skt, m_rd_buff, RD_BUFF, 0)) < 0 && errno == EINTR)
            ;
        if (r <= 0) {
            return -1;
        }
        m_cur_pos = 0;
        m_num_read = r;
        m_just_read = 1;
    } else {
        m_just_read = 0;
    }
    return m_rd_buff[m_cur_pos++];

}

void XpadClient::errmsg_handler(const string errmsg) {
    DEB_MEMBER_FUNCT();
    m_errorMessage = errmsg;
    DEB_TRACE() << m_errorMessage;
}

void XpadClient::debugmsg_handler(const string msg) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << msg;
    m_debugMessages.push_back(msg);
}

void XpadClient::timebar_handler(int done, int outoff, const string errmsg) {
    DEB_MEMBER_FUNCT();
    /* do nothing */
}

void XpadClient::error_handler(const string errmsg) {
    DEB_MEMBER_FUNCT();
    m_errorMessage = errmsg;
    DEB_TRACE() << m_errorMessage;
}

