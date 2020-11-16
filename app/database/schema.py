import cantools
import sqlite3

class TableWriter:

    CREATE_TABLE = "CREATE TABLE {} ({})"
    DEFAULT_DBC = "common-all/Data/2018CAR.dbc"

    def __init__(self, sqlite_db):
        self.conn = sqlite3.connect(sqlite_db)
        self.c = self.conn.cursor()

    def write_table(self, table_name, signals,
            primary_keys,
            additional_columns=[('timestamp', 'REAL')]):
        entries = []
        for entry in signals:
            if isinstance(entry, cantools.database.can.Signal):
                if entry.decimal.scale == round(entry.decimal.scale):
                    entries.append(str(entry.name) + ' INTEGER')
                else:
                    entries.append(str(entry.name) + ' REAL')
            else:
                # These signals are from the multiplexed values which are
                # all REALs
                entries.append(entry + ' REAL')

        if not isinstance(additional_columns, list):
            additional_columns = [additional_columns]

        for column in additional_columns:
            entries.append(str(column[0]) + ' ' + str(column[1]))

        entries.append("PRIMARY KEY ({})".format(','.join(primary_keys)))
        entries = ','.join(entries)
        try:
            self.c.execute(self.CREATE_TABLE.format(table_name, entries))
            self.tables_updated += 1
        except sqlite3.OperationalError as e:
            pass
            # print(self.CREATE_TABLE.format(table_name, entries))
            # print("Error using above SQL creating table: {}\n".format(e))

    def parse_dbc(self, dbc=None):
        if dbc is None:
            dbc = self.DEFAULT_DBC

        db = cantools.database.load_file(dbc)
        self.tables_updated = 0
        # Create a table for every message
        for message in db.messages:
            if not message.is_multiplexed():
                self.parse_message(message)
            else:
                self.parse_multiplexed_message(message)

        self.conn.commit()
        # print("Tables updated: {}".format(self.tables_updated))

    def parse_message(self, message):
        self.write_table(
            message.name,
            message.signals,
            ['timestamp']
        )

    def parse_multiplexed_message(self, message):
        count = 0
        signal_tree = message.signal_tree
        non_multiplexed_signals = []
        for signal in signal_tree:
            if isinstance(signal, dict):
                for key in signal:
                    multiplex_table_name = key
                    multiplexer_data = signal[key]
                    for value in multiplexer_data:
                        self.write_table(
                            message.name + str(value),
                            multiplexer_data[value],
                            ['timestamp']
                        )
                        count += 1
        return count

