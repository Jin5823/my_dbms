select count(distinct pid) from ((select tr.id as owner_id2 from Trainer as tr where hometown = 'Sangnok City') as t1 inner join (select owner_id, pid from CatchedPokemon) as t2 on t1.owner_id2 = t2.owner_id );
