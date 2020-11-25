select name from ((select pid from ((select owner_id, pid from CatchedPokemon) as t1 inner join (select * from Trainer where hometown in ('Sangnok City', 'Brown City')) as t2 on t1.owner_id = t2.id) group by pid having count(distinct hometown) = 2) as t3 inner join (select id, name from Pokemon) as t4 on t3.pid = t4.id) order by name asc;
