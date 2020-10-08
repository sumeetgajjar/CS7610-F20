#!/usr/bin/env python3
import logging
import os
import subprocess
import unittest
from concurrent.futures import ThreadPoolExecutor
from typing import Dict, List

import time


def configure_logging(level=logging.INFO):
    logger = logging.getLogger()
    logger.setLevel(level)

    log_formatter = logging.Formatter("[%(process)d] %(asctime)s [%(levelname)s] %(name)s: %(message)s")

    console_handler = logging.StreamHandler()
    console_handler.setFormatter(log_formatter)
    logger.addHandler(console_handler)


configure_logging()

NETWORK_BRIDGE = "cs7610-bridge"
NETWORK_BRIDGE_EXISTS_CMD = f'docker network ls --quiet --filter name={NETWORK_BRIDGE}'
NETWORK_BRIDGE_CREATE_CMD = f'docker network create --driver bridge {NETWORK_BRIDGE}'
RUNNING_CONTAINERS_CMD = 'docker ps -a --quiet --filter name=sumeet-g*'
STOP_CONTAINERS_CMD = 'docker stop {CONTAINERS}'
START_CONTAINER_CMD = "docker run --rm --detach" \
                      " --name {HOST} --network {NETWORK_BRIDGE} --hostname {HOST}" \
                      " -v {LOG_DIR}:/var/log/lab1 --env GLOG_log_dir=/var/log/lab1" \
                      " sumeet-g-prj1 {VERBOSE} --hostfile /lab1/hostfile {ARGS}"


class ProcessInfo:

    def __init__(self, return_code: int, stdout: str, stderr: str) -> None:
        super().__init__()
        self.return_code = return_code
        self.stdout = stdout
        self.stderr = stderr


class BaseSuite(unittest.TestCase):
    HOSTS = []
    LOG_ROOT = 'logs'

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
        logging.info(f"running cmd: {cmd}")
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
            p_create = cls.run_shell(NETWORK_BRIDGE_CREATE_CMD.split(" "))
            cls.assert_process_exit_status("network bridge create cmd", p_create)

    @classmethod
    def get_host_log_dir(cls, host: str) -> str:
        return os.path.abspath(f'{cls.LOG_ROOT}/{host}')

    @classmethod
    def get_verbose_logging_flag(cls):
        return f'--v {os.getenv("VERBOSE", 0)}'

    @classmethod
    def get_container_logs(cls, container_name: str):
        p = cls.run_shell(f"docker logs {container_name}")
        cls.assert_process_exit_status("docker logs cmd", p)
        raw_log = p.stdout + os.linesep + p.stderr
        log_lines = [line.strip() for line in raw_log.strip().split(os.linesep)]
        return log_lines

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
    def stop_running_containers(cls) -> None:
        p_run = cls.run_shell(RUNNING_CONTAINERS_CMD)
        cls.assert_process_exit_status("running containers cmd", p_run)
        if p_run.stdout.strip():
            running_containers = p_run.stdout.strip().split(os.linesep)
            logging.info(f"found running containers: {running_containers}")
            p_stop = cls.run_shell(STOP_CONTAINERS_CMD.format(CONTAINERS=" ".join(running_containers)))
            cls.assert_process_exit_status("stop containers cmd", p_stop)
        else:
            logging.info("no running containers found")


class MulticastSuite(BaseSuite):

    @classmethod
    def setUpClass(cls):
        return super().setUpClass()

    def setUp(self):
        self.stop_running_containers()

    def tearDown(self):
        self.stop_running_containers()

    def __get_app_args(self, host, msgCount=0, dropRate=0, delay=0, initiateSnapshotCount=0) -> Dict[str, str]:
        return {
            'HOST': host,
            'NETWORK_BRIDGE': NETWORK_BRIDGE,
            'LOG_DIR': self.get_host_log_dir(host),
            'VERBOSE': self.get_verbose_logging_flag(),
            'ARGS': f"--msgCount {msgCount}"
                    f" --dropRate {dropRate}"
                    f" --delay {delay}"
                    f" --initiateSnapshotCount {initiateSnapshotCount}"
        }

    def __get_delivery_order(self, host, expected_msg_count) -> List[str]:
        delivery_order = []
        while len(delivery_order) < expected_msg_count:
            logs = self.get_container_logs(host)
            delivery_order = [line.split("]")[1] for line in logs if "delivering" in line]
            logging.info(f"{len(delivery_order)} messages were delivered to {host}")
            time.sleep(4)

        return delivery_order

    def test_single_sender(self):
        msg_count = 10
        for host in self.HOSTS:
            logging.info(f"starting container for host: {host}")
            p_run = self.run_shell(START_CONTAINER_CMD.format(**self.__get_app_args(host, msgCount=msg_count)))
            self.assert_process_exit_status(f"{host} container run cmd", p_run)

        with ThreadPoolExecutor(len(self.HOSTS)) as executor:
            expected_msg_count = len(self.HOSTS) * msg_count
            futures = [executor.submit(self.__get_delivery_order, host, expected_msg_count) for host in self.HOSTS]
            delivery_orders = [future.result() for future in futures]
            for ix, order in enumerate(delivery_orders[:-1]):
                self.assertEqual(order, delivery_orders[ix + 1],
                                 f"order of process {ix} does not match with that of process {ix + 1}")


if __name__ == '__main__':
    unittest.main()
