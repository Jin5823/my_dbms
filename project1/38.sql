select name from Evolution as e1 left outer join Evolution as e2 on e1.after_id=e2.before_id inner join Pokemon as p1 on p1.id=e1.after_id where e2.before_id is null order by name asc;
