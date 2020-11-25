select type, count(*) as cnt from ((select pid from CatchedPokemon) as t1 inner join (select * from Pokemon) as t2 on t1.pid = t2.id) group by type order by type asc;
