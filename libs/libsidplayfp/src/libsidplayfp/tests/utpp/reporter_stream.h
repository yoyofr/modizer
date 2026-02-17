#pragma once
/*
  UTPP - A New Generation of UnitTest++
  (c) Mircea Neacsu 2017-2023

  See LICENSE file for full copyright information.
*/

/*!
  \file reporter_stream.h
  \brief Definition of UnitTest::ReporterStream class
*/

#include <iostream>
#include <iomanip>

namespace UnitTest {

/// A Reporter that sends results directly to an output stream
class ReporterStream : public Reporter
{
public:
  ReporterStream (std::ostream& strm = std::cout);

protected:
  void SuiteStart (const TestSuite& suite) override;
  void TestStart (const Test& test) override;
  void TestFinish (const Test& test) override;
  int SuiteFinish (const TestSuite& suite) override;

  void ReportFailure (const Failure& failure) override;
  int Summary () override;

  std::ostream& out;
};

/*!
  Constructor for a stream reporter.

  \param strm Output stream
*/
inline
ReporterStream::ReporterStream (std::ostream& strm)
  : out (strm)
{
}

/// If tracing is enabled, show a suite start message
inline
void ReporterStream::SuiteStart (const TestSuite& suite)
{
  Reporter::SuiteStart (suite);
  if (!trace)
    return;
  out << "Begin suite: " << suite.name << std::endl;
}

/// If tracing is enabled, show a test start message
inline
void ReporterStream::TestStart (const Test& test)
{
  Reporter::TestStart (test);
  if (!trace)
    return;
  out << "Start test: " << test.test_name () << std::endl;
}

/// If tracing is enabled, show a test finish message
inline
void ReporterStream::TestFinish (const Test& test)
{
  if (trace)
    std::cout << "End test: " << test.test_name () << std::endl;
  Reporter::TestFinish (test);
}

/// If tracing is enabled, show a suite finish message
inline
int ReporterStream::SuiteFinish (const TestSuite& suite)
{
  if (trace)
    out << "End suite: " << suite.name << std::endl;
  return Reporter::SuiteFinish (suite);
}


/*!
  Output a failure message. If a test is in progress (the normal case)
  the message includes the name of the test and suite.

  \param failure - the failure information (filename, line number and message)
*/
inline
void ReporterStream::ReportFailure (const Failure& failure)
{
  out << "Failure in ";
  if (CurrentTest)
  {
    if (CurrentSuite != DEFAULT_SUITE)
      out << "suite " << CurrentSuite << ' ';
    out << "test " << CurrentTest->test_name ();
  }

#if defined(__APPLE__) || defined(__GNUG__)
  out << std::endl << failure.filename << ":" << failure.line_number << ": error: "
    << failure.message << std::endl;
#else
  out << std::endl << failure.filename << "(" << failure.line_number << "): error: "
    << failure.message << std::endl;
#endif
  Reporter::ReportFailure (failure);
}

/*!
  Prints a test run summary including number of tests, number of failures,
  running time, etc.
*/
inline
int ReporterStream::Summary ()
{
  if (total_failed_count > 0)
  {
    out << "FAILURE: " << total_failed_count << " out of "
      << total_test_count << " tests failed (" << total_failures_count
      << " failures)." << std::endl;
  }
  else
    out << "Success: " << total_test_count << " tests passed." << std::endl;

  out.setf (std::ios::fixed);
  out << "Run time: " << std::setprecision (2) << total_time_msec / 1000. << std::endl;
  return Reporter::Summary ();
}

/*!
  This is only for compatibility with previous version.
  
  New code should use `ReporterStream`
*/
class ReporterStdout : public ReporterStream
{
public:
  ReporterStdout () : ReporterStream () {};
};

} //namespace UnitTest
