MESSAGE(STATUS "finding library : boost")

FIND_PATH(BOOST_INC NAMES boost)

FIND_LIBRARY(BOOST_SYSTEM     NAMES boost_system     boost_system-mt     boost_system-mt-x64)
FIND_LIBRARY(BOOST_THREAD     NAMES boost_thread     boost_thread-mt     boost_thread-mt-x64)
FIND_LIBRARY(BOOST_FILESYSTEM NAMES boost_filesystem boost_filesystem-mt boost_filesystem-mt-x64)
FIND_LIBRARY(BOOST_CHRONO     NAMES boost_chrono     boost_chrono-mt     boost_chrono-mt-x64)
FIND_library(BOOST_DATETIME   NAMES boost_date_time  boost_date_time-mt  boost_date_time-mt-x64)
