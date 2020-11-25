select name from CatchedPokemon as cp1 inner join Trainer as t1 on cp1.owner_id = t1.id group by name having count(pid) - count(distinct pid) >= 1 order by name asc;
