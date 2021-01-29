#!/usr/bin/python3

import argparse
from multiprocessing import Process

from can_monitor import CanMonitor
from dashboard import DashboardRunner
from database.database import Database

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='Run the WFE codebase')
    parser.add_argument('--monitor', help='Run the CAN Bus parser/monitor',
                        action='store_true')
    parser.add_argument('--database', help='Run the CAN DB',
                        action='store_true')
    parser.add_argument('--dashboard', help='Run the dashboard',
                        action='store_true')

    args = parser.parse_args()

    if args.monitor:
        monitor = CanMonitor()
        bus_p = Process(target=monitor.monitor_bus())
        bus_p.start()
        print("CAN monitor started.")

    if args.database:
        database = Database()
        database_p = Process(target=database.run())
        database_p.start()
        print("Database started.")

    if args.dashboard:
        dashboard = DashboardRunner()
        dashboard_p = Process(target=dashboard.run())
        dashboard_p.start()
        print("Dashboard started.")

