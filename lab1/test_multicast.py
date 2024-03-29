#!/usr/bin/env python3
import logging
import os
import subprocess
import unittest
from concurrent.futures import ThreadPoolExecutor
from typing import Dict, List, Callable


def configure_logging(level=logging.INFO):
    logger = logging.getLogger()
    logger.setLevel(level)

    log_formatter = logging.Formatter("[%(process)d] %(asctime)s [%(levelname)s] %(name)s: %(message)s")

    console_handler = logging.StreamHandler()
    console_handler.setFormatter(log_formatter)
    logger.addHandler(console_handler)


configure_logging(logging.DEBUG if os.getenv("VERBOSE", "0") == "1" else logging.INFO)

NETWORK_BRIDGE = "cs7610-bridge"
NETWORK_BRIDGE_EXISTS_CMD = f'docker network ls --quiet --filter name={NETWORK_BRIDGE}'
NETWORK_BRIDGE_CREATE_CMD = f'docker network create --driver bridge {NETWORK_BRIDGE}'
RUNNING_CONTAINERS_CMD = 'docker ps -a --quiet --filter name=sumeet-g*'
STOP_CONTAINERS_CMD = 'docker stop {CONTAINERS}'
REMOVE_CONTAINERS_CMD = 'docker rm {CONTAINERS}'
START_CONTAINER_CMD = "docker run --detach" \
                      " --name {HOST} --network {NETWORK_BRIDGE} --hostname {HOST}" \
                      " -v {LOG_DIR}:/var/log/lab1 --env GLOG_log_dir=/var/log/lab1" \
                      " sumeet-g-prj1 {VERBOSE} --hostfile /hostfile {ARGS}"


class ProcessInfo:

    def __init__(self, return_code: int, stdout: str, stderr: str) -> None:
        super().__init__()
        self.return_code = return_code
        self.stdout = stdout
        self.stderr = stderr


class BaseSuite(unittest.TestCase):
    HOSTS = []
    LOG_ROOT_DIR = 'logs'

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        with open('hostfile', 'r') as hostfile:
            cls.HOSTS = [host.strip() for host in hostfile.readlines()]

        logging.info(f'hosts: {cls.HOSTS}')
        cls.__create_logs_dir()
        cls.__setup_network_bridge()

    @classmethod
    def assert_process_exit_status(cls, name: str, p_info: ProcessInfo) -> None:
        assert p_info.return_code == 0, \
            f"{name} failed, returncode: {p_info.return_code}, stdout: {p_info.stdout}, stderr: {p_info.stderr}"

    @classmethod
    def run_shell(cls, cmd: str) -> ProcessInfo:
        logging.debug(f"running cmd: {cmd}")
        p = subprocess.run(cmd.split(" "), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return ProcessInfo(p.returncode, p.stdout.decode('UTF-8'), p.stderr.decode('UTF-8'))

    @classmethod
    def __setup_network_bridge(cls) -> None:
        p_exists = cls.run_shell(NETWORK_BRIDGE_EXISTS_CMD)
        cls.assert_process_exit_status("network bridge exists cmd", p_exists)
        if len(p_exists.stdout.strip()):
            logging.info(f"docker network bridge: {NETWORK_BRIDGE} exists")
        else:
            logging.info("docker network bridge does not exists, creating one")
            p_create = cls.run_shell(NETWORK_BRIDGE_CREATE_CMD)
            cls.assert_process_exit_status("network bridge create cmd", p_create)

    @classmethod
    def get_host_log_dir(cls, host: str) -> str:
        return os.path.abspath(f'{cls.LOG_ROOT_DIR}/{host}')

    @classmethod
    def get_verbose_logging_flag(cls) -> str:
        return f'--v {os.getenv("VERBOSE", 0)}'

    @classmethod
    def get_container_logs(cls, container_name: str) -> List[str]:
        p = cls.run_shell(f"docker logs {container_name}")
        cls.assert_process_exit_status("docker logs cmd", p)
        raw_log = p.stdout + os.linesep + p.stderr
        log_lines = [line.strip() for line in raw_log.strip().split(os.linesep)]
        return log_lines

    @classmethod
    def tail_container_logs(cls, container_name: str, callback: Callable[[str], bool]) -> None:
        continue_tailing = True
        with subprocess.Popen(['docker', 'logs', container_name, '-f'],
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE) as p:
            while continue_tailing:
                line = p.stderr.readline()
                if not line:
                    break

                line = line.decode('UTF-8')
                continue_tailing = callback(line)

            p.kill()

    @classmethod
    def __create_logs_dir(cls) -> None:
        for host in cls.HOSTS:
            host_log_dir = cls.get_host_log_dir(host)
            if os.path.isdir(host_log_dir):
                logging.info(f"log dir already exists: {host_log_dir}")
            else:
                logging.info(f"creating log dir: {host_log_dir}")
                os.makedirs(host_log_dir, exist_ok=True)

    @classmethod
    def stop_and_remove_running_containers(cls, remove_container=False) -> None:
        logging.info("stopping running containers")
        p_run = cls.run_shell(RUNNING_CONTAINERS_CMD)
        cls.assert_process_exit_status("running containers cmd", p_run)
        if p_run.stdout.strip():
            running_containers = p_run.stdout.strip().split(os.linesep)
            logging.info(f"found running containers: {running_containers}")
            p_stop = cls.run_shell(STOP_CONTAINERS_CMD.format(CONTAINERS=" ".join(running_containers)))
            cls.assert_process_exit_status("stop containers cmd", p_stop)
            if remove_container:
                p_remove = cls.run_shell(REMOVE_CONTAINERS_CMD.format(CONTAINERS=" ".join(running_containers)))
        else:
            logging.info("no running containers found")


class MulticastSuite(BaseSuite):

    @classmethod
    def setUpClass(cls):
        return super().setUpClass()

    def setUp(self) -> None:
        self.stop_and_remove_running_containers(remove_container=True)

    def tearDown(self) -> None:
        self.stop_and_remove_running_containers()

    def __get_app_args(self, host: str, senders: List[str],
                       msg_count, drop_rate, delay, initiate_snapshot_count) -> Dict[str, str]:
        return {
            'HOST': host,
            'NETWORK_BRIDGE': NETWORK_BRIDGE,
            'LOG_DIR': self.get_host_log_dir(host),
            'VERBOSE': self.get_verbose_logging_flag(),
            'ARGS': f" --senders {','.join(senders)}"
                    f" --msgCount {msg_count}"
                    f" --dropRate {drop_rate}"
                    f" --delay {delay}"
                    f" --initiateSnapshotCount {initiate_snapshot_count}"
        }

    @classmethod
    def __get_delivery_order(cls, host: str, expected_msg_count: int) -> List[str]:
        delivery_order = []

        def __callback(line: str) -> bool:
            line = line.strip()
            if "delivering" in line:
                delivery_msg = line.split("]")[1]
                delivery_order.append(delivery_msg)
                logging.info(f"found delivery message for host: {host}, message: {delivery_msg}")

            return len(delivery_order) < expected_msg_count

        cls.tail_container_logs(host, __callback)
        return delivery_order

    def __assert_ordering_is_same_for_all_processes(self, expected_msg_count: int) -> None:
        with ThreadPoolExecutor(len(self.HOSTS)) as executor:
            futures = [executor.submit(self.__get_delivery_order, host, expected_msg_count)
                       for host in self.HOSTS]
            delivery_orders = [future.result() for future in futures]

            for ix, delivery_order in enumerate(delivery_orders):
                self.assertTrue(delivery_order)
                self.assertEqual(delivery_orders[0], delivery_order,
                                 f"order of process {ix} does not match with that of process {0}")

    def __test_wrapper(self, senders: List[str], msg_count=0, drop_rate=0.0, delay=0,
                       snapshot_initiator=None,
                       initiate_snapshot_count=0) -> None:
        logging.info(f"senders for the test: {senders}")
        logging.info(f"args: msgCount: {msg_count}, dropRate: {drop_rate}, delay: {delay}, "
                     f"initiateSnapshotCount: {initiate_snapshot_count}")
        for host in self.HOSTS:
            host_initiate_snapshot_count = initiate_snapshot_count if host == snapshot_initiator else 0
            logging.info(f"starting container for host: {host}")
            p_run = self.run_shell(
                START_CONTAINER_CMD.format(**self.__get_app_args(host, senders=senders, msg_count=msg_count,
                                                                 drop_rate=drop_rate, delay=delay,
                                                                 initiate_snapshot_count=host_initiate_snapshot_count)))
            self.assert_process_exit_status(f"{host} container run cmd", p_run)

        expected_msg_count = len(senders) * msg_count
        logging.info("grepping message delivery in process logs")
        self.__assert_ordering_is_same_for_all_processes(expected_msg_count)

    def test_one_sender(self):
        self.__test_wrapper(senders=self.HOSTS[0:1], msg_count=4)

    def test_two_senders(self):
        self.__test_wrapper(senders=self.HOSTS[0:2], msg_count=4)

    def test_all_senders(self):
        self.__test_wrapper(senders=self.HOSTS, msg_count=4)

    def test_drop_half_messages(self):
        self.__test_wrapper(senders=self.HOSTS[0:1], msg_count=2, drop_rate=0.5)

    def test_drop_majority_messages(self):
        self.__test_wrapper(senders=self.HOSTS[0:1], msg_count=2, drop_rate=0.75)

    def test_delay_messages(self):
        self.__test_wrapper(senders=self.HOSTS[0:1], msg_count=2, delay=2000)

    def test_drop_delay_messages(self):
        self.__test_wrapper(senders=self.HOSTS[0:1], msg_count=2, drop_rate=0.25, delay=2000)

    def test_snapshot_with_one_sender(self):
        self.__test_wrapper(senders=self.HOSTS[0:1], msg_count=8,
                            snapshot_initiator=self.HOSTS[0],
                            initiate_snapshot_count=3)

    def test_snapshot_with_two_senders(self):
        self.__test_wrapper(senders=self.HOSTS[0:2], msg_count=8,
                            snapshot_initiator=self.HOSTS[1],
                            initiate_snapshot_count=3)

    def test_snapshot_with_all_senders(self):
        self.__test_wrapper(senders=self.HOSTS, msg_count=8,
                            snapshot_initiator=self.HOSTS[-1],
                            initiate_snapshot_count=3)


if __name__ == '__main__':
    unittest.main()
