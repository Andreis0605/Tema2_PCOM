import subprocess
import sys
import signal
import time
import os
import pprint
import json

from contextlib import contextmanager
from subprocess import Popen, PIPE, STDOUT
from os import path
from time import sleep

# default port for the  server
port = "12345"

# default IP for the server
ip = "127.0.0.1"

# default UDP client path
udp_client_path = "pcom_hw2_udp_client"

# default size of test output line
test_output_line_size = 60

####### Test utils #######
# dictionary containing test IDs and their statuses
tests = {
  "compile": "not executed",
  "server_start": "not executed",
  "c1_start": "not executed",
  "data_unsubscribed": "not executed",
  "c1_subscribe_all": "not executed",
  "data_subscribed": "not executed",
  "c1_stop": "not executed",
  "c1_restart": "not executed",
  "data_no_clients": "not executed",
  "same_id": "not executed",
  "c2_start": "not executed",
  "c2_subscribe": "not executed",
  "c2_stop": "skipped",
  "c2_subscribe_plus_wildcard": "not executed",
  "c2_subscribe_star_wildcard": "not executed",
  "c2_subscribe_compound_wildcard": "not executed",
  "c2_subscribe_wildcard_set_inclusion": "not executed",
  "quick_flow": "not executed",
  "server_stop": "not executed",
}

def pass_test(test):
  """Marks a test as passed."""
  tests[test] = "passed"

def fail_test(test):
  """Marks a test as failed."""
  tests[test] = "failed"

def print_test_results():
  """Prints the results for all the tests."""
  print("")
  print("RESULTS")
  print("-------")
  for test in tests:
    dots = test_output_line_size - len(test) - len(tests.get(test))
    print(test, end="")
    print('.' * dots, end="")
    print(tests.get(test))

####### Topic utils #######
class Topic:
  """Class that represents a subscription topic with data."""

  def __init__(self, name, category, value):
    self.name = name
    self.category = category
    self.value = value

  def print(self):
    """Prints the current topic and data in the expected format."""
    return self.name + " - " + self.category + " - " + self.value

  @staticmethod
  def generate_topics(use_wildcard = False):
    """Generates topics with data for various kinds."""
    ret = []
    if not use_wildcard:
      ret.append(Topic("a_non_negative_int", "INT", "10"))
      ret.append(Topic("a_negative_int", "INT", "-10"))
      ret.append(Topic("a_larger_value", "INT", "1234567890"))
      ret.append(Topic("a_large_negative_value", "INT", "-1234567890"))
      ret.append(Topic("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwx", "INT", "10"))
      ret.append(Topic("that_is_small_short_real", "SHORT_REAL", "2.30"))
      ret.append(Topic("that_is_big_short_real", "SHORT_REAL", "655.05"))
      ret.append(Topic("that_is_integer_short_real", "SHORT_REAL", "17"))
      ret.append(Topic("float_seventeen", "FLOAT", "17"))
      ret.append(Topic("float_minus_seventeen", "FLOAT", "-17"))
      ret.append(Topic("a_strange_float", "FLOAT", "1234.4321"))
      ret.append(Topic("a_negative_strange_float", "FLOAT", "-1234.4321"))
      ret.append(Topic("a_subunitary_float", "FLOAT", "0.042"))
      ret.append(Topic("a_negative_subunitary_float", "FLOAT", "-0.042"))
      ret.append(Topic("ana_string_announce", "STRING", "Ana are mere"))
      ret.append(Topic("huge_string", "STRING", "abcdefghijklmnopqrstuvwxyz"))
    else:
      ret.append(Topic("upb/precis/elevator/1/people", "INT", "5"))
      ret.append(Topic("upb/precis/elevator/1/floor", "INT", "4"))
      ret.append(Topic("upb/precis/elevator/2/people", "INT", "0"))
      ret.append(Topic("upb/precis/elevator/2/floor", "INT", "0"))
      ret.append(Topic("upb/precis/100/temperature", "SHORT_REAL", "24.30"))
      ret.append(Topic("upb/precis/100/humidity", "SHORT_REAL", "29.87"))
      ret.append(Topic("upb/ec/100/temperature", "SHORT_REAL", "21.30"))
      ret.append(Topic("upb/ec/100/humidity", "SHORT_REAL", "26.11"))
      ret.append(Topic("upb/ec/100/pressure", "FLOAT", "1013.2512"))
      ret.append(Topic("upb/precis/100/pressure", "FLOAT", "1043.1249"))
      ret.append(Topic("upb/precis/100/schedule/monday/8", "STRING", "Protocoale De Comunicatie"))
      ret.append(Topic("upb/ec/100/schedule/tuesday/12", "STRING", "Protocoale De Comunicatie"))
      ret.append(Topic("upb/precis/100/schedule/monday/8", "STRING", "Protocoale De Comunicatie"))
      ret.append(Topic("upb/ec/100/schedule/tuesday/12", "STRING", "Protocoale De Comunicatie"))
    return ret

####### Process utils#######
class Process:
  """Class that represents a process which can be controlled."""

  def __init__(self, command, cwd=""):
    self.command = command
    self.started = False
    self.cwd = cwd

  def start(self):
    """Starts the process."""
    try:
      if self.cwd == "":
        self.proc = Popen(self.command, universal_newlines=True, stdin=PIPE, stdout=PIPE, stderr=PIPE)
      else:
        self.proc = Popen(self.command, universal_newlines=True, stdin=PIPE, stdout=PIPE, stderr=PIPE, cwd=self.cwd)
      self.started = True
    except FileNotFoundError as e:
      print(e)
      quit()

  def finish(self):
    """Terminates the process and waits for it to finish."""
    if self.started:
      self.proc.terminate()
      self.proc.wait(timeout=1)
      self.started = False

  def send_input(self, proc_in):
    """Sends input and a newline to the process."""
    if self.started:
      self.proc.stdin.write(proc_in + "\n")
      self.proc.stdin.flush()

  def get_output(self):
    """Gets one line of output from the process."""
    if self.started:
      return self.proc.stdout.readline()
    else:
      return ""

  def get_output_timeout(self, tout):
    """Tries to get one line of output from the process with a timeout."""
    if self.started:
      with timeout(tout):
        try:
          return self.proc.stdout.readline()
        except TimeoutError as e:
          return "timeout"
    else:
      return ""

  def get_error(self):
    """Gets one line of stderr from the process."""
    if self.started:
      return self.proc.stderr.readline()
    else:
      return ""

  def get_error_timeout(self, tout):
    """Tries to get one line of stderr from the process with a timeout."""
    if self.started:
      with timeout(tout):
        try:
          return self.proc.stderr.readline()
        except TimeoutError as e:
          return "timeout"
    else:
      return ""

  def is_alive(self):
    """Checks if the process is alive."""
    if self.started:
      return self.proc.poll() is None
    else:
      return False

####### Helper functions #######
@contextmanager
def timeout(time):
  """Raises a TimeoutError after a duration specified in seconds."""
  signal.signal(signal.SIGALRM, raise_timeout)
  signal.alarm(time)

  try:
    yield
  except TimeoutError:
    pass
  finally:
    signal.signal(signal.SIGALRM, signal.SIG_IGN)

def raise_timeout(signum, frame):
  """Raises a TimeoutError."""
  raise TimeoutError

def make_target(target):
  """Runs a makefile for a given target."""
  subprocess.run(["make " + target], shell=True)
  return path.exists(target)

def make_clean():
  """Runs the clean target in a makefile."""
  subprocess.run(["make clean"], shell=True)

def exit_if_condition(condition, message):
  """Exits and prints the test results if a condition is true."""
  if condition:
    print(message)
    make_clean()
    print_test_results()
    quit()

def get_procfs_values(rmem):
  """Reads TCP buffer sizes from procfs."""
  path = "/proc/sys/net/ipv4/tcp_" + ("rmem" if rmem else "wmem")
  #path = "tcp_" + ("rmem" if rmem else "wmem")
  file = open(path, "r")
  values = file.readline().split()
  if len(values) < 3:
    print("Error: could not read correctly from procfs")
    return ["error"]

  return values

def set_procfs_values(rmem, values):
  """Writes TCP buffer sizes to procfs."""
  path = "/proc/sys/net/ipv4/tcp_" + ("rmem" if rmem else "wmem")
  #path = "tcp_" + ("rmem" if rmem else "wmem")

  if not os.access(path, os.W_OK):
    print("Error: not enough permissions to write to procfs")
    return False

  file = open(path, "w")
  file.write(values[0] + " " + values[1] + " " + values[2])

  return True

####### Test helper functions #######
def run_udp_client(mode=True, types=["0"], input_file="sample_payloads.json"):
  """Runs a UDP client which generates messages on one or multiple topics."""
  if mode:
    udpcl = Process(["python3", "udp_client.py", "--input_file", input_file, ip, port], udp_client_path)
    udpcl.start()
    for i in range(19):
      outudp = udpcl.get_output_timeout(1)
    udpcl.finish()
  else:
    udpcl = Process(["python3", "udp_client.py", "--mode", "manual", "--input_file", input_file, ip, port], udp_client_path)
    udpcl.start()
    sleep(1)
    for type in types:
      udpcl.send_input(type)
      sleep(1)
    udpcl.send_input("exit")
    udpcl.finish()

def start_and_check_client(server, id, restart=False, test=True):
  """Starts a TCP client and checks that it starts."""
  if test:
    fail_test("c" + id + ("_restart" if restart else "_start"))

  print("Starting subscriber C" + id)
  client = Process(["./subscriber", "C" + id, ip, port])
  client.start()
  outs = server.get_output_timeout(2)

  success = True

  # check if the client successfully connected to the server
  if not client.is_alive():
    print("Error: subscriber C" + id + " is not up")
    success = False

  if not outs.startswith("New client C" + id + " connected from"):
    print("Error: server did not print that C" + id + " is connected")
    success = False
  
  if success and test:
    pass_test("c" + id + ("_restart" if restart else "_start"))

  return client, success

def check_subscriber_output(c, client_id, target):
  """Compares the output of a TCP client with an expected string."""
  outc = c.get_output_timeout(1)
  if target not in outc:
    print("Error: C" + client_id + " output should contain [" + target + "], is actually [" + outc.rstrip() + "]")
    return False

  return True

def check_subscriber_stop(server, c, id):
  """Stops a TCP client and checks that it stops."""
  print("Disconnecting subscriber C" + id)
  c.send_input("exit")
  sleep(1)

  # check that the process is no longer alive
  outs = server.get_output_timeout(1)
  message = "Client C" + id + " disconnected."
  if outs.rstrip() != message or c.is_alive():
    print("Error: client C" + id + " not disconnected")
    return False

  return True

def check_two_subscribers(c1, c2, topics, topic_id):
  """Compares the output of two TCP clients with an expected string."""
  topic = topics[topic_id]

  # generate one message for the topic
  print("Generating one message for topic " + topic.name)
  run_udp_client(False, [str(topic_id)])

  # check that both subscribers receive the message correctly
  target = topic.print()
  success = check_subscriber_output(c1, "1", target)
  return check_subscriber_output(c2, "2", target) and success

def subscribe_to_topic(c, topic_name="", wildcard=""):
  """Sends subscribe command on a client"""
  c.send_input("subscribe " + topic_name + wildcard)
  outc = c.get_output_timeout(1)
  if not outc.startswith("Subscribed to topic"):
    print("Error: C not subscribed to topic " + wildcard)
    return -1

####### Test functions #######
def run_test_compile():
  """Tests that the server and subscriber compile."""
  fail_test("compile")
  print("Compiling")
  exit_if_condition(not make_target("server"), "Error: server could not be built")
  exit_if_condition(not make_target("subscriber"), "Error: subscriber could not be built")
  pass_test("compile")

def run_test_server_start():
  """Tests that the server starts correctly."""
  fail_test("server_start")
  print("Starting the server")
  server = Process(["./server", port])
  server.start()
  sleep(1)
  exit_if_condition(not server.is_alive(), "Error: server is not up")
  pass_test("server_start")
  return server

def run_test_c1_start(server):
  """Tests that a subscriber C1 starts correctly."""
  return start_and_check_client(server, "1")

def run_test_data_unsubscribed(server, c1):
  """Tests that messages from topics not subscribed to are not received."""
  fail_test("data_unsubscribed")

  # generate one message for each topic
  print("Generating one message for each topic")
  run_udp_client()

  # check that the server and C1 print nothing
  outs = server.get_output_timeout(1)
  outc1 = c1.get_output_timeout(1)

  failed = False

  if outs != "timeout":
    print("Error: server printing [" + outs.rstrip() + "]")
    failed = True

  if outc1 != "timeout":
    print("Error: C1 printing [" + outc1.rstrip() + "]")
    failed = True

  if not failed:
    pass_test("data_unsubscribed")

def run_test_c1_subscribe_all(server, c1, topics):
  """Tests that subscriber C1 can subscribe to all topics."""
  fail_test("c1_subscribe_all")
  print("Subscribing C1 to all topics")

  failed = False
  for topic in topics:
    c1.send_input("subscribe " + topic.name)
    outc1 = c1.get_output_timeout(1)
    if not outc1.startswith("Subscribed to topic"):
      print("Error: C1 not subscribed to all topics")
      failed = True
      break

  if not failed:
    pass_test("c1_subscribe_all")

def run_test_data_subscribed(server, c1, topics):
  """Tests that subscriber C1 receives messages on subscribed topics."""
  fail_test("data_subscribed")

  # generate one message for each topic
  print("Generating one message for each topic")
  run_udp_client()

  # check that C1 receives all the messages correctly
  success = True
  for topic in topics:
    success = check_subscriber_output(c1, "1", topic.print()) and success

  if success:
    pass_test("data_subscribed")

def run_test_c1_stop(server, c1):
  """Tests that subscriber C1 stops correctly."""
  fail_test("c1_stop")

  if check_subscriber_stop(server, c1, "1"):
    pass_test("c1_stop")
    return True

  return False

def run_test_c1_restart(server):
  """Tests that subscriber C1 restarts correctly."""

  # generate one message for each topic
  print("Generating one message for each topic")
  run_udp_client()
  sleep(1)

  # restart and check subscriber C1
  return start_and_check_client(server, "1", True)

def run_test_data_no_clients(c1):
  """Tests that subscriber C1 doesn't receive anything from the server upon restart."""
  fail_test("data_no_clients")

  if c1.get_output_timeout(1) == "timeout":
    pass_test("data_no_clients")

def run_test_same_id(server):
  """Tests that the server doesn't accept two subscribers with the same ID."""
  fail_test("same_id")
  print("Starting another subscriber with ID C1")

  c1bis = Process(["./subscriber", "C1", ip, port])
  c1bis.start()
  sleep(1)

  outs = server.get_output_timeout(2)
  success = True

  if c1bis.is_alive():
    print("Error: second subscriber C1 is up")
    success = False

  if not outs.startswith("Client C1 already connected."):
    print("Error: server did not print that C1 is already connected")
    success = False

  if success:
    pass_test("same_id")

def run_test_c2_start(server):
  """Tests that a subscriber C2 starts correctly."""
  return start_and_check_client(server, "2")

def run_test_c2_subscribe(c2, topics):
  """Tests that subscriber C2 can subscribe to a topic."""
  fail_test("c2_subscribe")
  topic = topics[0]

  print("Subscribing C2 to topic " + topic.name)

  c2.send_input("subscribe " + topic.name)
  outc2 = c2.get_output_timeout(1)
  if not outc2.startswith("Subscribed to topic"):
    print("Error: C2 not subscribed to topic " + topic.name)
    return

  pass_test("c2_subscribe")

def run_test_c2_stop(server, c2):
  """Tests that subscriber C2 stops correctly."""
  fail_test("c2_stop")

  if check_subscriber_stop(server, c2, "2"):
    pass_test("c2_stop")
    return True

  return False

def run_test_quick_flow(c1, topics):
  """Tests that subscriber C1 receives many messages in quick succession on subscribed topics."""
  fail_test("quick_flow")

  rmem = get_procfs_values(True)
  wmem = get_procfs_values(False)

  if rmem[0] == "error" or wmem[0] == "error":
    return

  if not set_procfs_values(True, ["5", "5", "5"]):
    return
  
  if not set_procfs_values(False, ["5", "5", "5"]):
    set_procfs_values(True, rmem)
    return

  # generate one message for each topic 30 times in a row
  print("Generating one message for each topic 30 times in a row")
  for i in range(30):
    run_udp_client()

  # check that C1 receives all the messages correctly
  success = True
  for i in range(30):
    for topic in topics:
      success = check_subscriber_output(c1, "1", topic.print()) and success

  if success:
    pass_test("quick_flow")

  set_procfs_values(True, rmem)
  set_procfs_values(False, wmem)

def run_test_server_stop(server, c1):
  """Tests that the server stops correctly."""
  fail_test("server_stop")
  print("Stopping the server")

  server.send_input("exit")
  sleep(1)

  success = True

  if server.is_alive():
    print("Error: server is still up")
    success = False

  if c1.is_alive():
    print("Error: C1 is still up")
    success = False

  if success:
    pass_test("server_stop")

def run_test_c2_subscribe_plus_wildcard(c2, topics):
  """Tests that subscriber C2 can subscribe to a topic with wildcard."""
  # setup the test and the wildcard flow
  fail_test("c2_subscribe_plus_wildcard")

  # subscribe to topics
  wildcard = '+/ec/100/pressure'
  print("Subscribing C2 to topics " + wildcard)
  if subscribe_to_topic(c2, "", wildcard) == -1:
    return

  # generate messages on topics
  topics_to_generate = ["8", "9"]
  run_udp_client(False, topics_to_generate, "sample_wildcard_payloads.json")

  # run the checks
  success = check_subscriber_output(c2, "2", topics[8].print())
  success = success and check_subscriber_output(c2, "2", "timeout")

  # unsubscribe from topics
  c2.send_input("unsubscribe " + wildcard)
  c2.get_output_timeout(1)

  # subscribe to topics
  wildcard = 'upb/+/100/pressure'
  print("Subscribing C2 to topics " + wildcard)
  if subscribe_to_topic(c2, "", wildcard) == -1:
    return
  
  # generate messages on topics
  run_udp_client(False, topics_to_generate, "sample_wildcard_payloads.json")

  # run the checks
  success = success and check_subscriber_output(c2, "2", topics[8].print())
  success = success and check_subscriber_output(c2, "2", topics[9].print())

  #unsubscribe from topics
  c2.send_input("unsubscribe " + wildcard)
  c2.get_output_timeout(1)

  # subscribe to topics
  wildcard = 'upb/ec/100/+'
  print("Subscribing C2 to topics " + wildcard)
  if subscribe_to_topic(c2, "", wildcard) == -1:
    return
  
  # generate messages on topics
  topics_to_generate = ["6", "7", "5"]
  run_udp_client(False, topics_to_generate, "sample_wildcard_payloads.json")

  # run the checks
  success = success and check_subscriber_output(c2, "2", topics[6].print())
  success = success and check_subscriber_output(c2, "2", topics[7].print())
  success = success and check_subscriber_output(c2, "2", "timeout")

  # unsubscribe from topics
  c2.send_input("unsubscribe " + wildcard)
  c2.get_output_timeout(1)

  if success:
    pass_test("c2_subscribe_plus_wildcard")


def run_test_c2_subscribe_star_wildcard(c2, topics):
  """Tests that subscriber C2 can subscribe to a topic with wildcard."""
  # setup the test
  fail_test("c2_subscribe_star_wildcard")

  # subscribe to topics
  wildcard = '*/pressure'
  print("Subscribing C2 to topics " + wildcard)
  if subscribe_to_topic(c2, "", wildcard) == -1:
      return

  # generate messages on topics
  topics_to_generate = ["8", "9", "7"]
  run_udp_client(False, topics_to_generate, "sample_wildcard_payloads.json")

  # run the checks
  success = check_subscriber_output(c2, "2", topics[8].print())
  success = success and check_subscriber_output(c2, "2", topics[9].print())
  success = check_subscriber_output(c2, "2", "timeout")

  # unsubscribe from topics
  c2.send_input("unsubscribe " + wildcard)
  c2.get_output_timeout(1)

  # subscribe to topics
  wildcard = 'upb/precis/elevator/*/floor'
  print("Subscribing C2 to topics " + wildcard)
  if subscribe_to_topic(c2, "", wildcard) == -1:
    return
    
  # generate messages on topics
  topics_to_generate = ["1", "3", "2"]
  run_udp_client(False, topics_to_generate, "sample_wildcard_payloads.json")

  # run the checks
  success = success and check_subscriber_output(c2, "2", topics[1].print())
  success = success and check_subscriber_output(c2, "2", topics[3].print())
  success = success and check_subscriber_output(c2, "2", "timeout")

  # unsubscribe from topics
  c2.send_input("unsubscribe " + wildcard)
  c2.get_output_timeout(1)

  # subscribe to topics
  wildcard = 'upb/precis/*'
  print("Subscribing C2 to topics " + wildcard)
  if subscribe_to_topic(c2, "", wildcard) == -1:
    return
    
  # generate messages on topics
  topics_to_generate = ["1", "4", "6"]
  run_udp_client(False, topics_to_generate, "sample_wildcard_payloads.json")

  # run the checks
  success = success and check_subscriber_output(c2, "2", topics[1].print())
  success = success and check_subscriber_output(c2, "2", topics[4].print())
  success = success and check_subscriber_output(c2, "2", "timeout")

  # unsubscribe from topics
  c2.send_input("unsubscribe " + wildcard)
  c2.get_output_timeout(1)

  # subscribe to topics
  wildcard = '*'
  print("Subscribing C2 to topics " + wildcard)
  if subscribe_to_topic(c2, "", wildcard) == -1:
    return

  # generate messages on topics
  run_udp_client(input_file="sample_wildcard_payloads.json")

  # run the checks
  for topic in topics:
    success = check_subscriber_output(c2, "2", topic.print()) and success

  # unsubscribe from topics
  c2.send_input("unsubscribe " + wildcard)
  c2.get_output_timeout(1)

  if success:
    pass_test("c2_subscribe_star_wildcard")

def run_test_c2_subscribe_compound_wildcard(c2, topics):
  """Tests that subscriber C2 can subscribe to a topic with wildcard."""
  # setup the test
  fail_test("c2_subscribe_compound_wildcard")

  # subscribe to topics
  wildcard = 'upb/+/100/+'
  print("Subscribing C2 to topics " + wildcard)
  if subscribe_to_topic(c2, "", wildcard) == -1:
      return

  # generate messages on topics
  topics_to_generate = ["4", "5", "10"]
  run_udp_client(False, topics_to_generate, "sample_wildcard_payloads.json")

  # run the checks
  success = check_subscriber_output(c2, "2", topics[4].print())
  success = success and check_subscriber_output(c2, "2", topics[5].print())
  success = success and check_subscriber_output(c2, "2", "timeout")

  # unsubscribe from topics
  c2.send_input("unsubscribe " + wildcard)
  c2.get_output_timeout(1)

  # subscribe to topics
  wildcard = 'upb/+/100/*'
  print("Subscribing C2 to topics " + wildcard)
  if subscribe_to_topic(c2, "", wildcard) == -1:
      return

  # generate messages on topics
  topics_to_generate = ["9", "10", "0"]
  run_udp_client(False, topics_to_generate, "sample_wildcard_payloads.json")

  # run the checks
  success = success and check_subscriber_output(c2, "2", topics[9].print())
  success = success and check_subscriber_output(c2, "2", topics[10].print())
  success = success and check_subscriber_output(c2, "2", "timeout")

  # unsubscribe from topics
  c2.send_input("unsubscribe " + wildcard)
  c2.get_output_timeout(1)

  # subscribe to topics
  wildcard = '*/100/+'
  print("Subscribing C2 to topics " + wildcard)
  if subscribe_to_topic(c2, "", wildcard) == -1:
    return
    
  # generate messages on topics
  topics_to_generate = ["4", "5", "10", "0"]
  run_udp_client(False, topics_to_generate, "sample_wildcard_payloads.json")

  # run the checks
  success = success and check_subscriber_output(c2, "2", topics[4].print())
  success = success and check_subscriber_output(c2, "2", topics[5].print())
  success = success and check_subscriber_output(c2, "2", "timeout")
  success = success and check_subscriber_output(c2, "2", "timeout")

  # unsubscribe from topics
  c2.send_input("unsubscribe " + wildcard)
  c2.get_output_timeout(1)

  # subscribe to topics
  wildcard = '*/100/*'
  print("Subscribing C2 to topics " + wildcard)
  if subscribe_to_topic(c2, "", wildcard) == -1:
    return
    
  # generate messages on topics
  run_udp_client(False, topics_to_generate, "sample_wildcard_payloads.json")
  
  # run the checks
  success = success and check_subscriber_output(c2, "2", topics[4].print())
  success = success and check_subscriber_output(c2, "2", topics[5].print())
  success = success and check_subscriber_output(c2, "2", topics[10].print())
  success = success and check_subscriber_output(c2, "2", "timeout")

  # unsubscribe from topics
  c2.send_input("unsubscribe " + wildcard)
  c2.get_output_timeout(1)

  if success:
    pass_test("c2_subscribe_compound_wildcard")

def run_test_c2_subscribe_wildcard_set_inclusion(c2, topics):
  """Tests that subscriber C2 can subscribe to a topic with wildcard."""
  # setup the test
  fail_test("c2_subscribe_wildcard_set_inclusion")

  # subscribe to topics
  wildcards = ['upb/precis/100/+']
  print("Subscribing C2 to topics " + wildcards[0])
  if subscribe_to_topic(c2, "", wildcards[0]) == -1:
    return
    
  wildcards.append('upb/precis/100/*')
  print("Subscribing C2 to topics " + wildcards[1])
  if subscribe_to_topic(c2, "", wildcards[1]) == -1:
    return

  # generate messages on topics
  run_udp_client(False, ["4"], "sample_wildcard_payloads.json")

  # run the checks
  topic = topics[4]
  success = check_subscriber_output(c2, "2", topic.print())
  success = success and check_subscriber_output(c2, "2", "timeout")
  # success = c2.get_output_timeout(1) == "" and success

  # unsubscribe from topics
  c2.send_input("unsubscribe " + wildcards[0])
  c2.get_output_timeout(1)

  c2.send_input("unsubscribe " + wildcards[1])
  c2.get_output_timeout(1)

  if success:
    pass_test("c2_subscribe_wildcard_set_inclusion")

def h2_test():
  """Runs all the tests."""

  # clean up
  make_clean()

  # generate the topics
  topics = Topic.generate_topics()
  wildcard_topics = Topic.generate_topics(True)

  # build the two binaries and check
  run_test_compile()

  # start the server and check it is running
  server = run_test_server_start()

  # start a subscriber C1 and check it is running
  c1, success = run_test_c1_start(server)

  if success:    
    # generate data and check that it isn't received by C1
    run_test_data_unsubscribed(server, c1)

    # subscribe C1 to all topics and verify
    run_test_c1_subscribe_all(server, c1, topics)

    # generate messages on all topics and check that C1 receives them
    run_test_data_subscribed(server, c1, topics)

    # stop C1 and check it exits correctly
    success = run_test_c1_stop(server, c1)

    if success:
      # restart C1 and check that it starts properly
      c1, success = run_test_c1_restart(server)

      if success:
        # check that C1 doesn't receive anything from the server
        run_test_data_no_clients(c1)

        # connect a client with the same ID as C1 and check that it fails
        run_test_same_id(server)

        # start a subscriber C2 and check it is running
        c2, success = run_test_c2_start(server)

        if success:
          # subscribe C2 to a single topic and check
          run_test_c2_subscribe(c2, topics)

          # subscribe C2 to topics containing single + wildcard and check
          run_test_c2_subscribe_plus_wildcard(c2, wildcard_topics)

          # subscribe C2 to topics containing single * wildcard and check
          run_test_c2_subscribe_star_wildcard(c2, wildcard_topics)

          # subscribe C2 to topics containing multiple wildcards and check
          run_test_c2_subscribe_compound_wildcard(c2, wildcard_topics)

          # subscribe C2 to topics containing wildcards and check for duplicate messages
          run_test_c2_subscribe_wildcard_set_inclusion(c2, wildcard_topics)

          # stop C2 and check it exits correctly
          success = run_test_c2_stop(server, c2)

    # send all types of message 30 times in quick succesion and check
    run_test_quick_flow(c1, topics)

  # close the server and check that C1 also closes
  run_test_server_stop(server, c1)

  # clean up
  make_clean()

  # print test results
  print_test_results()

# run all tests
h2_test()
