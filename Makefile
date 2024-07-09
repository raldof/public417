EXTENSION   = chessgame
MODULES 	= chessgame
DATA        = chessgame--1.0.sql chessgame.control

PG_CONFIG ?= pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
