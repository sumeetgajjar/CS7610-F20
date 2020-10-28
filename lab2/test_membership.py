#!/usr/bin/env python3
import logging
import os
import subprocess
import time
import unittest
from concurrent.futures import ThreadPoolExecutor
from typing import Dict, List, Callable, Any


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
                      " -v {LOG_DIR}:/var/log/lab2 --env GLOG_log_dir=/var/log/lab2" \
                      " sumeet-g-prj2 {VERBOSE} --hostfile /hostfile {ARGS}"


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
    def tail_container_logs(cls, container_name: str, callback: Callable[[str, Any], bool],
                            *args, **kwargs) -> None:
        continue_tailing = True
        with subprocess.Popen(['docker', 'logs', container_name, '-f'],
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE) as p:
            while continue_tailing:
                line = p.stderr.readline()
                if not line:
                    break

                line = line.decode('UTF-8')
                continue_tailing = callback(line, *args, **kwargs)

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


class MembershipSuite(BaseSuite):
    VIEW_INSTALLATION_SUBSTR = "new view installed"
    PROCESS_CRASHED_SUBSTR = "not reachable"

    @classmethod
    def setUpClass(cls):
        super().setUpClass()

    def setUp(self) -> None:
        self.stop_and_remove_running_containers(remove_container=True)

    def tearDown(self) -> None:
        self.stop_and_remove_running_containers()

    def __get_app_args(self, host: str) -> Dict[str, str]:
        return {
            'HOST': host,
            'NETWORK_BRIDGE': NETWORK_BRIDGE,
            'LOG_DIR': self.get_host_log_dir(host),
            'VERBOSE': self.get_verbose_logging_flag(),
            'ARGS': ''
        }

    def __wait_for_view_installation(self, host: str, required_view_log_count: int):
        logging.debug(f"waiting for new view installation in host: {host}")
        view_log = []

        def __callback(log_line: str, view_log_count: int) -> bool:
            if self.VIEW_INSTALLATION_SUBSTR in log_line:
                view_log.append(log_line)

            return len(view_log) < view_log_count

        self.tail_container_logs(host, __callback, view_log_count=required_view_log_count)

    def __start_all_containers(self):
        for ix, host in enumerate(self.HOSTS):
            logging.info(f"starting container for host: {host}")
            p_run = self.run_shell(START_CONTAINER_CMD.format(**self.__get_app_args(host)))
            self.assert_process_exit_status(f"{host} container run cmd", p_run)
            self.__wait_for_view_installation(host, required_view_log_count=1)

    def __wait_for_peer_crash(self, host: str, peer_crash_count: int):
        logging.debug(f"waiting for peer crash detection in host: {host}")
        peer_crash_msg = []

        def __call_back(log_line: str) -> bool:
            if self.PROCESS_CRASHED_SUBSTR in log_line:
                peer_crash_msg.append(log_line)

            return len(peer_crash_msg) < peer_crash_count

        self.tail_container_logs(host, __call_back)
        logging.info(f"number of crashes detected by {host}: {len(peer_crash_msg)}")

    def test_case_1(self):
        self.__start_all_containers()

        for ix, host in enumerate(self.HOSTS):
            expected_view_installations = len(self.HOSTS) - ix
            actual_view_installations = sum([1 for line in self.get_container_logs(host)
                                             if self.VIEW_INSTALLATION_SUBSTR in line])
            logging.info(f"found {actual_view_installations} view installations for {host}")
            self.assertEqual(expected_view_installations, actual_view_installations)

    def test_case_2(self):
        leader_host = self.HOSTS[0]
        peer_to_crash = self.HOSTS[-1]

        self.__start_all_containers()
        logging.info(f"crashing peer: {peer_to_crash}")
        p_stop = self.run_shell(STOP_CONTAINERS_CMD.format(CONTAINERS=peer_to_crash))
        self.assert_process_exit_status("crash peer cmd", p_stop)
        logging.info("waiting for peer crashed messages")
        self.__wait_for_peer_crash(leader_host, 1)

        # sleeping for (HEARTBEAT_INTERVAL_MS * 3) so all peers can detect the crash
        time.sleep(1.5)

        peers_detecting_process_crash = self.HOSTS[:-1]
        actual_process_crashes = []
        for host in peers_detecting_process_crash:
            actual_process_crashes.append(sum([1 for line in self.get_container_logs(host)
                                               if self.PROCESS_CRASHED_SUBSTR in line]))

        expected_process_crashes = [1 for _ in peers_detecting_process_crash]
        self.assertListEqual(expected_process_crashes, actual_process_crashes, f"process crash count mismatch")

    def test_case_3(self):
        leader_host = self.HOSTS[0]
        self.__start_all_containers()
        for ix, peer_to_stop in enumerate(reversed(self.HOSTS[1:])):
            logging.info(f"crashing peer: {peer_to_stop}")
            p_stop = self.run_shell(STOP_CONTAINERS_CMD.format(CONTAINERS=peer_to_stop))
            self.assert_process_exit_status("crash peer cmd", p_stop)
            logging.info("waiting for peer crashed messages")
            self.__wait_for_peer_crash(leader_host, ix + 1)

            # sleeping for (HEARTBEAT_INTERVAL_MS * 3) so all peers can detect the crash
            time.sleep(1.5)

        peers_detecting_process_crash = self.HOSTS[:-1]
        actual_process_crashes = []
        for host in peers_detecting_process_crash:
            actual_process_crashes.append(sum([1 for line in self.get_container_logs(host)
                                               if self.PROCESS_CRASHED_SUBSTR in line]))

        expected_process_crashes = list(reversed(range(1, len(self.HOSTS))))
        self.assertListEqual(expected_process_crashes, actual_process_crashes)


if __name__ == '__main__':
    unittest.main()
