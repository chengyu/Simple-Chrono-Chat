SimpleChronoChat: a toy example to show how to use NDN ChronoSync
=================================================================================

The ChronoSync library (https://github.com/named-data/ChronoSync) is a synchronization
library for distributed realtime applications for NDN. Many distributed applications can
utilize this library to exploits the features of the Named Data Networking architecture
to efficiently synchronize the state of a dataset among a distributed group of users.

However, this library does not contain a simple example to allow users follow up. To
demonstrate how it works, this SimpleChronoChat is implemented.

SimpleChronoSync is an open source project licensed under GPL 3.0 (see `COPYING.md` for more
detail).

Feedback
--------

Please submit any bugs or issues to the **ChronoSync** issue tracker:

* http://redmine.named-data.net/projects/chronosync

Installation instructions
-------------------------

### Prerequisites

Required:

* [ndn-cxx and its dependencies](http://named-data.net/doc/ndn-cxx/)
* Boost libraries
* [ChronoSync] (https://github.com/named-data/ChronoSync)

### Build

To build SimpleChronoChat from the source:

    ./waf configure
    ./waf
    sudo ./waf install
