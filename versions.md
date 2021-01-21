Forthcoming
-----------

* TopicDataType interface extended (ABI break)
* Upgrade to Quality Level 1
* New DataWriter API for loaning samples (DataWriter API extension)
* Calls to `logInfo`, `logWarning`, and `logError` are forced to end with semicolon.

Version 2.1.0
-------------

This minor release is API compatible with the previous minor release, but introduces **ABI breaks** on
two of the three public APIs:
* Methods and attributes have been added on several classes of the DDS-PIM high-level API, so indexes of
  symbols on dynamic libraries may have changed
* Methods and attributes have been added on several classes of the RTPS low-level API, so indexes of
  symbols on dynamic libraries may have changed
* Old Fast-RTPS high-level API remains ABI compatible.

Users of the RTPS low-level API should also be aware of the following **API deprecations**:
* History::reserve_Cache has been deprecated
  * Methods RTPSWriter::new_change or RTPSReader::reserveCache should be used instead
* History::release_Cache has been deprecated
  * Methods RTPSWriter::release_change or RTPSReader::releaseCache should be used instead

This release adds the follwing **features**:
* Support persistence for large data
* Added support for `on_requested_incompatible_qos` and `on_offered_incompatible_qos`
* SKIP_DEFAULT_XML environment variable
* Added FORCE value to THIRDPARTY cmake options
* New log consumer (StdOutErrConsumer)
* Added methods to get qos defined in XML Profile
* Support for persistence on TRANSIENT_LOCAL

It also includes the following **improvements**:
* Internal refactor for intra-process performance boost
* Allow usage of foonathan/memory library built without debug tool
* Large data support on performance tests
* Reduced flakiness of several tests

Some important **bugfixes** are also included:
* Fixed behavior of several DDS API methods
* Fixed interoperability issues with RTI connext
* Fixed DLL export of some methods
* Avoid redefinition of compiler defined macros
* Fixed some intra-process related segmentation faults and deadlocks
* Fixed large data payload protection issues on intra-process
* Fixed C++17 and VS 2019 warnings
* Fixed linker problems on some platforms
* Fixed transient local retransmission after participant drop
* Fixed assertion failure on persistent writers

Version 2.0.2
-------------

This release includes the following **improvements**:

* Improve QNX support
* Security improvements
* Fast DDS Quality Declaration (QL 2)
* Large traffic reduction when using Discovery Server (up to 85-90% for large deployments)
* Configuration of Clients of Discovery Server using an environment variable
* A CLI for Fast DDS:
  * This can be used to launch a discovery server
  * Clean SHM directories with one command
* Shared memory transport enabled by default
* Solved edge-case interoperability issue with CycloneDDS

Version 2.0.1
-------------

This release includes the following bug fixes:

* Fixed sending GAPs to late joiners
* Fixed asserting liveliness on data reception
* Avoid calling OpenSSL_add_all_algorithms when not required

Other improvements:

* Fixing warnings

Version 2.0.0
-------------

This release has the following **API breaks**:

* eClock API, which was deprecated on v1.9.1, has been removed
* `eprosima::fastrtps::rtps::RTPSDomain::createParticipant` methods now have an additional first argument `domain_id`
* Data member `domainId` has been removed from `eprosima::fastrtps::rtps::RTPSParticipantAttributes` and added to
  `eprosima::fastrtps::ParticipantAttributes`

Users should also be aware of the following **deprecation announcement**:

* All classes inside the namespace `eprosima::fastrtps` should be considered deprecated.
  Equivalent functionality is offered through namespace `eprosima::fastdds`.
* Namespaces beneath `eprosima::fastrtps` are not included in this deprecation, i.e.
  `eprosima::fastrtps::rtps` can still be used)

This release adds the following **features**:

* Added support for register/unregister/dispose instance
* Added DDS compliant API. This new API exposes all the functionality of the Publisher-Subscriber Fast RTPS API
  adhering to the `Data Distribution Service (DDS) version 1.4 specification <https://www.omg.org/spec/DDS/1.4>`_
* Added Security Logging Plugin (contributed by Cannonical Ltd.)
* Bump to FastCDR v1.0.14

It also includes the following bug fixes and improvements:

* Support for OpenSSL 1.1.1d and higher
* Support for latest versions of gtest
* Support for FreeBSD
* Fault tolerance improvements to Shared Memory transport
* Fixed segfault when no network interfaces are detected
* Correctly ignoring length of `PID_SENTINEL` on parameter list
* Improved traffic on PDP simple mode
* Reduced CPU and memory usage

Version 1.10.0
--------------

This release adds the following features:

* New built-in :ref:`comm-transports-shm`
* Transport API refactored to support locator iterators
* Added subscriber API to retrieve info of first non-taken sample
* Added parameters to fully avoid dynamic allocations
* History of built-in endpoints can be configured
* Bump to FastCDR v1.0.13.
* Bump to Fast-RTPS-Gen v1.0.4.
* Require CMake 3.5 but use policies from 3.13

It also includes the following bug fixes and improvements:

* Fixed alignment on parameter lists
* Fixed error sending more than 256 fragments.
* Fix handling of STRICT_REALTIME.
* Fixed submessage_size calculation on last data_frag.
* Solved an issue when recreating a publishing participant with the same GUID.
* Solved an issue where a publisher could block on write for a long time when a new
  subscriber (late joiner) is matched, if the publisher had already sent a large number
  of messages.
* Correctly handling the case where lifespan expires at the same time on several samples.
* Solved some issues regarding liveliness on writers with no readers.
* Correctly removing changes from histories on keyed topics.
* Not reusing cache change when sample does not fit.
* Fixed custom wait_until methods when time is in the past.
* Several data races and ABBA locks fixed.
* Reduced CPU and memory usage.
* Reduced flakiness of liveliness tests.
* Allow for more use cases on performance tests.

Several bug fixes on discovery server:

* Fixed local host communications.
* Correctly trimming server history.
* Fixed backup server operation.
* Fixed timing issues.

Version 1.9.4
-------------

This release adds the following features:

* Intra-process delivery mechanism is now active by default.
* Synchronous writers are now allowed to send fragments.
* New memory mode DYNAMIC_RESERVE on history pool.
* Performance tests can now be run on Windows and Mac.
* XML profiles for requester and replier.

It also includes the following bug fixes and improvements:

* Bump to FastCDR v1.0.12.
* Bump to Fast-RTPS-Gen v1.0.3.
* Fixed deadlock between PDP and StatefulReader.
* Improved CPU usage and allocations on timed events management.
* Performance improvements on reliable writers.
* Fixing bugs when Intra-process delivery is activated.
* Reducing dynamic allocations and memory footprint.
* Improvements and fixes on performance tests.
* Other minor bug fixes and improvements.

Version 1.9.3
-------------

This release adds the following features:

* Participant discovery filtering flags.
* Intra-process delivery mechanism opt-in.

It also includes the following bug fixes and improvements:

* Bump to Fast-RTPS-Gen v1.0.2.
* Bring back compatibility with XTypes 1.1 on PID_TYPE_CONSISTENCY.
* Ensure correct alignment when reading a parameter list.
* Add CHECK_DOCUMENTATION *cmake* option.
* EntityId_t and GuidPrefix_t have now their own header files.
* Fix potential race conditions and deadlocks.
* Improve the case where *check_acked_status* is called between reader matching process and its acknack reception.
* RTPSMessageGroup_t instances now use the thread-local storage.
* FragmentedChangePitStop manager removed.
* Remove the data fragments vector on CacheChange_t.
* Only call find_package for TinyXML2 if third-party options are off
* Allow XMLProfileManager methods to not show error log messages if a profile is not found.


Version 1.9.2
-------------

This release includes the following feature:

* Multiple initial PDP announcements.
* Flag to avoid builtin multicast.

It also adds the following bug fixes and improvements:

* Bump to Fast-RTPS-Gen v1.0.1.
* Bump to IDL-Parser v1.0.1.

Version 1.9.1
-------------

This release includes the following features:

* Fast-RTPS-Gen is now an independent project.
* Header **eClock.h** is now marked as deprecated.

It also adds the following bug fixes and improvements:

* Bump to FastCDR v1.0.11.
* Installation from sources documentation fixed.
* Fixed assertion on WriterProxy.
* Fixed potential fall through while parsing Parameters.
* Removed deprecated guards causing compilation errors in some 32 bits platforms.
* *addTOCDRMessage* method is now exported in the DLL, fixing issues related with Parameters' constructors.
* Improve windows performance by avoiding usage of *_Cnd_timedwait* method.
* Fixed reported communication issues by sending multicast through *localhost* too.
* Fixed potential race conditions and deadlocks.
* Eliminating use of *acceptMsgDirectTo*.
* Discovery Server framework reconnect/recreate strategy.
* Removed unused folders.
* Restored subscriber API.
* SequenceNumber_t improvements.
* Added STRICT_REALTIME *cmake* option.
* SubscriberHistory improvements.
* Assertion of participant liveliness by receiving RTPS messages from the remote participant.
* Fixed error while setting next deadline event in *create_new_change_with_params*.

Version 1.9.0
-------------

This release includes the following features:

* Partial implementation of allocation QoS.
* Implementation of Discovery Server.
* Implementation of non-blocking calls.

It also adds the following bug fixes and improvements:

* Added sliding window to BitmapRange.
* Modified default behavior for unknown writers.
* A `Flush()` method was added to the logger to ensure all info is logged.
* A test for loading `Duration_t` from XML was added.
* Optimized WLP when removing local writers.
* Some liveliness tests were updated so that they are more stable on Windows.
* A fix was added to `CMakeLists.txt` for installing static libraries.
* A fix was added to performance tests so that they can run on the RT kernel.
* Fix for race condition on built-in protocols creation.
* Fix for setting *nullptr* in a *fixed_string*.
* Fix for v1.8.1 not building with -DBUILD_JAVA=ON.
* Fix for GAP messages not being sent in some cases.
* Fix for coverity report.
* Several memory issues fixes.
* `fastrtps.repos` file was updated.
* Documentation for building with Colcon was added.
* Change CMake configuration directory if INSTALLER_PLATFORM is set.
* IDL sub-module updated to current version.

Version 1.8.4
-------------

This release adds the following **feature**:

* XML profiles for `requester` and `replier`

It also has the following **important bug fixes**:

* Solved an issue when recreating a publishing participant with the same GUID (either on purpose or by chance)
* Solved an issue where a publisher could block on `write` for a long time when, after a large number of samples
  have been sent, a new subscriber is matched.

Version 1.8.3
-------------

This release adds the following bug fixes and improvements:

* Fix serialization of TypeConsistencyEnforcementQosPolicy.
* Bump to Fast-RTPS-Gen v1.0.2.
* Bump to IDL-Parser v1.0.1.

Version 1.8.2
-------------

This release includes the following features:

* Modified unknown writers default behavior.
* Multiple initial PDP announcements.
* Flag to avoid builtin multicast.
* *STRICT_REALTIME* compilation flag.

It also adds the following bug fixes and improvements:

* Fix for setting `nullptr` in a fixed string.
* Fix for not sending GAP in several cases.
* Solve *Coverity* report issues.
* Fix issue of *fastrtpsgen* failing to open *IDL.g4* file.
* Fix unnamed lock in *AESGCMGMAC_KeyFactory.cpp*.
* Improve *XMLProfiles* example.
* Multicast is now sent through *localhost* too.
* *BitmapRange* now implements sliding window.
* Improve *SequenceNumber_t* struct.
* Participant's liveliness is now asserted when receiving any RTPS message.
* Fix leak on RemoteParticipantLeaseDuration.
* Modified default values to improve behavior in *Wi-Fi* scenarios.
* *SubscriberHistory* improvements.
* Removed use of *acceptMsgDirectTo*.
* *WLP* improvements.

Version 1.8.1
-------------

This release includes the following features:

* Implementation of :ref:`livelinessqospolicy` QoS.

It also adds the following bug fixes and improvements:

* Fix for get_change on history, which was causing issues during discovery.
* Fix for announcement of participant state, which was sending ParticipantBuiltinData twice.
* Fix for closing multicast UDP channel.
* Fix for race conditions in SubscriberHistory, UDPTransportInterface and StatefulReader.
* Fix for lroundl error on Windows in Time_t.
* CDR & IDL submodules update.
* Use of java 1.8 or greater for fastrtpsgen.jar generation.

Version 1.8.0
-------------

This release included the following features:

* Implementation of IDL 4.2.
* Implementation of :ref:`deadlineqospolicy` QoS.
* Implementation of :ref:`lifespanqospolicy` QoS.
* Implementation of :ref:`disablepositiveacksqospolicy` QoS.
* Secure sockets on TCP transport (:ref:`TLS`).

It also adds the following improvements and bug fixes:

* Real-time improvements: non-blocking write calls for best-effort writers, addition of fixed size strings,
  fixed size bitmaps, resource limited vectors, etc.
* Duration parameters now use nanoseconds.
* Configuration of participant mutation tries.
* Automatic calculation of the port when a value of 0 is received on the endpoint custom locators.
* Non-local addresses are now filtered from whitelists.
* Optimization of check for acked status for stateful writers.
* Linked libs are now not exposed when the target is a shared lib.
* Limitation on the domain ID has been added.
* UDP non-blocking send is now optional and configurable via XML.
* Fix for non-deterministic tests.
* Fix for ReaderProxy history being reloaded incorrectly in some cases.
* Fix for RTPS domain hostid being potentially not unique.
* Fix for participants with different lease expiration times failing to reconnect.

**Known issues**

* When using TPC transport, sometimes callbacks are not invoked when removing a participant due to a bug in ASIO.

Version 1.7.2
-------------

This release fixes an important bug:

* Allocation limits on subscribers with a KEEP_LAST QoS was taken from resource limits configuration
  and didn't take history depth into account.

It also has the following improvements:

* Vendor FindThreads.cmake from CMake 3.14 release candidate to help with sanitizers.
* Fixed format of gradle file.

Some other minor bugs and performance improvements.

Version 1.7.1
-------------

This release included the following features:

* LogFileConsumer added to the logging system.
* Handle FASTRTPS_DEFAULT_PROFILES_FILE environment variable indicating the default profiles XML file.
* XML parser made more restrictive and with better error messages.

It also fixes some important bugs:
* Fixed discovery issues related to the selected network interfaces on Windows.
* Improved discovery times.
* Workaround ASIO issue with multicast on QNX systems.
* Improved TCP transport performance.
* Improved handling of key-only data submessages.

Some other minor bugs and performance improvements.

**KNOWN ISSUES**

* Allocation limits on subscribers with a KEEP_LAST QoS is taken from resource limits configuration
  and doesn't take history depth into account.

Version 1.7.0
-------------

This release included the following features:

* :ref:`comm-transports-tcp`.
* :ref:`dynamic-types`.
* Security 1.1 compliance.

Also bug fixing, allocation and performance improvements.

Version 1.6.0
-------------

This release included the following features:

* :ref:`persistence`.
* Security access control plugin API and builtin :ref:`access-permissions` plugin.

Also bug fixing.

Version 1.5.0
-------------

This release included the following features:

* Configuration of Fast RTPS entities through XML profiles.
* Added heartbeat piggyback support.

Also bug fixing.

Version 1.4.0
-------------

This release included the following:

* Added secure communications.
* Removed all Boost dependencies. Fast RTPS is not using Boost libraries anymore.
* Added compatibility with Android.
* Bug fixing.

Version 1.3.1
-------------

This release included the following:

* New examples that illustrate how to tweak Fast RTPS towards different applications.
* Improved support for embedded Linux.
* Bug fixing.

Version 1.3.0
-------------

This release introduced several new features:

* Unbound Arrays support: Now you can send variable size data arrays.
* Extended Fragmentation Configuration: It allows you to setup a Message/Fragment max size different to the standard
  64Kb limit.
* Improved logging system: Get even more introspection about the status of your communications system.
* Static Discovery: Use XML to map your network and keep discovery traffic to a minimum.
* Stability and performance improvements: A new iteration of our built-in performance tests will make benchmarking
  easier for you.
* ReadTheDocs Support: We improved our documentation format and now our installation and user manuals are available
  online on ReadTheDocs.

Version 1.2.0
-------------

This release introduced two important new features:

* Flow Controllers: A mechanism to control how you use the available bandwidth avoiding data bursts.
  The controllers allow you to specify the maximum amount of data to be sent in a specific period of time.
  This is very useful when you are sending large messages requiring fragmentation.
* Discovery Listeners: Now the user can subscribe to the discovery information to know the entities present in the
  network (Topics, Publishers & Subscribers) dynamically without prior knowledge of the system.
  This enables the creation of generic tools to inspect your system.

But there is more:

* Full ROS2 Support: Fast RTPS is used by ROS2, the upcoming release of the Robot Operating System (ROS).
* Better documentation: More content and examples.
* Improved performance.
* Bug fixing.
